"""Socket-level MOVE fan-out experiment. Not a Unity frame-rate or Internet CCU test.
Requires Python 3.10+ and psutil. All accounts and CSV state are synthetic.
"""
import argparse, asyncio, csv, hashlib, json, math, platform, shutil, statistics, subprocess, sys, time
from pathlib import Path
import psutil

def fixture(path,n):
    path.mkdir(parents=True,exist_ok=True)
    tables={
      'UsersAccount':('id,password,nickname',lambda i:f'u{i},synthetic-only,n{i}'),
      'UserProfile':('id,level,exp,icon,money0,money1,emo0,emo1,emo2,emo3,balloon',lambda i:f'u{i},1,0,0,100000,0,0,1,2,3,0'),
      'UserCharacters':('id,'+','.join(f'char{k}' for k in range(7)),lambda i:f'u{i},'+','.join(['1']*7)),
      'UserBalloon':('id,'+','.join(f'balloon{k}' for k in range(10)),lambda i:f'u{i},'+','.join(['1']*10)),
      'UserIcon':('id,'+','.join(f'icon{k}' for k in range(10)),lambda i:f'u{i},'+','.join(['1']*10)),
      'UserEmotes':('id,'+','.join(f'emo{k}' for k in range(36)),lambda i:f'u{i},'+','.join(['1']*36)),
      'UserWinLossStats':('id,winCount,LoseCount,'+','.join(f'char{k}_{x}' for k in range(7) for x in ['win','lose']),lambda i:f'u{i},'+','.join(['0']*16))}
    for name,(header,row) in tables.items():
        (path/(name+'.csv')).write_text(header+'\n'+'\n'.join(row(i) for i in range(n))+'\n',encoding='ascii')
    for name,field in [('StoreCharacters','char'),('StoreBalloon','balloon'),('StoreIcons','icon'),('StoreEmotes','emo')]:
        (path/(name+'.csv')).write_text(f'{field}Index,price,priceType\n0,5000,0\n1,5000,0\n',encoding='ascii')
    (path/'CharStat.csv').write_text('CharIndex,Health,Attack\n'+'\n'.join(f'{i},100,10' for i in range(7))+'\n')

class Peer:
    def __init__(self,i,reader,writer): self.i=i;self.r=reader;self.w=writer
    async def send(self,s):self.w.write(s.encode());await self.w.drain()
    async def until(self,prefix,timeout=5):
        async def read():
            while True:
                line=await self.r.readline()
                if not line:raise ConnectionError('server closed')
                if line.startswith(prefix.encode()):return line.decode().strip()
        return await asyncio.wait_for(read(),timeout)

async def start(exe,port,folder,n):
    fixture(folder,n)
    log=(folder/'server.log').open('wb')
    proc=subprocess.Popen([str(exe)],cwd=folder,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    peers=[]
    try:
        for attempt in range(100):
            if proc.poll() is not None:raise RuntimeError('server startup failed: '+str(proc.returncode))
            try:r,w=await asyncio.open_connection('127.0.0.1',port);break
            except OSError:await asyncio.sleep(.05)
        else:raise TimeoutError('listen')
        peers.append(Peer(0,r,w))
        for i in range(1,n):
            r,w=await asyncio.open_connection('127.0.0.1',port);peers.append(Peer(i,r,w))
        return proc,log,peers
    except:
        proc.kill();proc.wait();log.close();raise

async def setup(peers,room_size=4):
    for p in peers:
        await p.send(f'LOGIN|u{p.i},synthetic-only\n');await p.until('LOGIN_SUCCESS|')
    for p in peers:
        room=p.i//room_size
        if p.i%room_size==0:
            await p.send(f'CREATE_ROOM|r{room}|Map1||0\n');await p.until('CREATE_ROOM_SUCCESS|')
        else:
            await p.send(f'ENTER_ROOM|r{room}||0\n');await p.until('ENTER_ROOM_SUCCESS|')

async def stop(proc,log,peers):
    # Stop this isolated server before closing sockets; this is not a shutdown test.
    if proc.poll() is None:proc.terminate()
    await asyncio.to_thread(proc.wait)
    for p in peers:p.w.close()
    await asyncio.gather(*(p.w.wait_closed() for p in peers),return_exceptions=True)
    log.close()

def percentile(values,q):
    if not values:return None
    a=sorted(values);return a[max(0,math.ceil(len(a)*q)-1)]

async def measure(exe,port,folder,n,hz,seconds,warmup):
    proc,log,peers=await start(exe,port,folder,n)
    samples=[];sent={};seen=set();cross=0;duplicates=0;malformed=0;received=0;bytes_in=0;errors=[];lag=[]
    info=psutil.Process(proc.pid);client=psutil.Process();telemetry=[]
    async def reader(p):
        nonlocal cross,duplicates,malformed,received,bytes_in
        try:
            while True:
                line=await p.r.readline()
                if not line:return
                if not line.startswith(b'MOVE_RESULT|'):continue
                try:
                    nick,x,z=line.decode().strip().split('|',1)[1].split(',');sender=int(nick[1:]);seq=int(float(x))
                except (ValueError,UnicodeError):malformed+=1;continue
                key=(sender,seq)
                if key not in sent:continue
                received+=1;bytes_in+=len(line)
                if sender//4!=p.i//4:cross+=1;continue
                k=(sender,seq,p.i)
                if k in seen:duplicates+=1;continue
                seen.add(k);samples.append((sender,seq,p.i,(time.perf_counter()-sent[key])*1000))
        except Exception as e:errors.append(type(e).__name__)
    async def monitor():
        while True:
            try:telemetry.append({'t':time.perf_counter(),'rss':info.memory_info().rss,'threads':info.num_threads()})
            except psutil.Error:return
            await asyncio.sleep(.1)
    readers=[];mon=None
    try:
        await setup(peers);readers=[asyncio.create_task(reader(p)) for p in peers];mon=asyncio.create_task(monitor())
        seq=0
        async def phase(duration,record):
            nonlocal seq
            begin=time.perf_counter();ticks=int(duration*hz)
            for tick in range(ticks):
                deadline=begin+tick/hz
                await asyncio.sleep(max(0,deadline-time.perf_counter()))
                if record:lag.append(max(0,time.perf_counter()-deadline)*1000)
                seq+=1
                for p in peers:
                    if record:sent[p.i,seq]=time.perf_counter()
                    p.w.write(f'MOVE|n{p.i}|{seq},0\n'.encode())
                await asyncio.gather(*(p.w.drain() for p in peers))
            await asyncio.sleep(max(0,begin+duration-time.perf_counter()))
            return time.perf_counter()-begin
        await phase(warmup,False);telemetry.clear()
        cpu0=sum(info.cpu_times()[:2]);client0=sum(client.cpu_times()[:2]);t0=time.perf_counter()
        elapsed=await phase(seconds,True)
        cpu=sum(info.cpu_times()[:2])-cpu0;client_cpu=sum(client.cpu_times()[:2])-client0
        await asyncio.sleep(2)
        times=[s[3] for s in samples]
        result={'clients':n,'rooms':n//4,'players_per_room':4,'hz_per_client':hz,'duration_s':elapsed,'warmup_s':warmup,'drain_s':2,
          'sent_messages':len(sent),'expected_in_room_deliveries':len(sent)*3,'received_in_room_deliveries':len(seen),
          'missing_in_room_deliveries':len(sent)*3-len(seen),'cross_room_deliveries':cross,'duplicates':duplicates,'malformed':malformed,
          'all_received_deliveries':received,'application_bytes_received':bytes_in,'offered_messages_per_s':len(sent)/elapsed,
          'p50_ms':percentile(times,.5),'p95_ms':percentile(times,.95),'p99_ms':percentile(times,.99),'max_ms':max(times,default=0),
          'server_cpu_core_percent':cpu/elapsed*100,'generator_cpu_core_percent':client_cpu/elapsed*100,
          'server_peak_rss_mib':max((v['rss'] for v in telemetry),default=0)/1048576,'server_peak_threads':max((v['threads'] for v in telemetry),default=0),
          'generator_schedule_lag_p99_ms':percentile(lag,.99),'reader_errors':errors,'server_alive':proc.poll() is None}
        with (folder/'samples.csv').open('w',newline='') as f:
            w=csv.writer(f);w.writerow(['sender','sequence','receiver','delivery_ms']);w.writerows(samples)
        (folder/'telemetry.json').write_text(json.dumps(telemetry),encoding='utf-8')
        (folder/'summary.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        return result
    finally:
        for t in readers:t.cancel()
        if mon:mon.cancel()
        await asyncio.gather(*readers,*([mon] if mon else []),return_exceptions=True)
        await stop(proc,log,peers)

async def main(a):
    build=Path(a.build).resolve();out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=True)
    meta=json.loads((build/'build.json').read_text());exe=build/'bin/TeamOFYServer.exe'
    meta['environment']={'os':platform.platform(),'python':sys.version,'logical_cpus':psutil.cpu_count(),'physical_cpus':psutil.cpu_count(logical=False),'ram_gib':round(psutil.virtual_memory().total/2**30,2),'transport':'TCP, IPv4 loopback; server and generator on the same host','cpu_percent_definition':'100% equals one logical core','latency_definition':'before sender write to observer complete line receipt, same-host perf_counter; not server-only latency','tcp_nodelay':'asyncio sets TCP_NODELAY on client connections; server default unchanged','measurement_date':time.strftime('%Y-%m-%d')}
    (out/'manifest.json').write_text(json.dumps(meta,indent=2),encoding='utf-8')
    results=[]
    for n in a.clients:
        for repeat in range(1,a.repeats+1):
            folder=out/f'c{n}-r{repeat}'
            r=await measure(exe,meta['port'],folder,n,a.hz,a.seconds,a.warmup);r['repeat']=repeat;results.append(r)
            (out/'results.json').write_text(json.dumps(results,indent=2),encoding='utf-8');print(json.dumps(r),flush=True)
            await asyncio.sleep(.3)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--build',required=True);p.add_argument('--out',required=True)
    p.add_argument('--clients',nargs='+',type=int,default=[4,16,32,64]);p.add_argument('--repeats',type=int,default=3)
    p.add_argument('--hz',type=int,default=20);p.add_argument('--seconds',type=float,default=10);p.add_argument('--warmup',type=float,default=2)
    a=p.parse_args();assert all(n>0 and n%4==0 for n in a.clients);asyncio.run(main(a))
