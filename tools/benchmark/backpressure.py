"""Actual-server experiment: a stopped reader in room 0 while room 1 receives MOVE probes."""
import asyncio,argparse,json,time,os,socket,csv
from pathlib import Path
import psutil
from run import start,setup,stop,percentile

async def experiment(build,folder,pressure):
 build=Path(build).resolve();folder=Path(folder).resolve();meta=json.loads((build/'build.json').read_text())
 prior=os.environ.get('BNB_TRANSPORT_STATS');os.environ['BNB_TRANSPORT_STATS']=str(folder/'transport.jsonl')
 try:proc,log,peers=await start(build/'bin/TeamOFYServer.exe',meta['port'],folder,8)
 finally:
  if prior is None:os.environ.pop('BNB_TRANSPORT_STATS',None)
  else:os.environ['BNB_TRANSPORT_STATS']=prior
 sent={};records=[];duplicates=0;seen=set();tasks=[];flood=None;flood_count=0;rss=[];threads=[]
 info=psutil.Process(proc.pid)
 async def reader(p):
  nonlocal duplicates
  try:
   while True:
    line=await p.r.readline()
    if not line:return
    if p.i in [5,6,7] and line.startswith(b'MOVE_RESULT|n4,'):
     seq=int(float(line.decode().split(',')[1]));key=(p.i,seq)
     if seq in sent:
      if key in seen:duplicates+=1
      else:seen.add(key);records.append((p.i,seq,(time.perf_counter()-sent[seq])*1000))
  except (OSError,ValueError):pass
 async def flooding():
  nonlocal flood_count
  payload='ROOM_MESSAGE|r0:n1:'+'x'*2048+'\n';begin=time.perf_counter()
  try:
   for tick in range(350):
    await asyncio.sleep(max(0,begin+tick*.02-time.perf_counter()))
    for _ in range(10):peers[1].w.write(payload.encode());flood_count+=1
    await peers[1].w.drain()
  except OSError:pass
 async def monitor():
  while True:
   try:rss.append(info.memory_info().rss);threads.append(info.num_threads())
   except psutil.Error:return
   await asyncio.sleep(.05)
 try:
  await setup(peers)
  if pressure:
   peers[0].w.get_extra_info('socket').setsockopt(socket.SOL_SOCKET,socket.SO_RCVBUF,1024)
   peers[0].w.transport.pause_reading()
  tasks=[asyncio.create_task(reader(p)) for p in peers if not (pressure and p.i==0)]
  tasks.append(asyncio.create_task(monitor()))
  cpu0=sum(info.cpu_times()[:2]);t0=time.perf_counter()
  if pressure:flood=asyncio.create_task(flooding())
  await asyncio.sleep(1)
  began=time.perf_counter()
  for seq in range(100):
   await asyncio.sleep(max(0,began+seq*.05-time.perf_counter()))
   sent[seq]=time.perf_counter();await peers[4].send(f'MOVE|n4|{seq},0\n')
  await asyncio.sleep(1)
  elapsed=time.perf_counter()-t0;cpu=(sum(info.cpu_times()[:2])-cpu0)/elapsed*100
  samples=[r[2] for r in records]
  result={'pressure':pressure,'clients':8,'rooms':2,'probe_requests':100,'expected_deliveries':300,'received_deliveries':len(seen),'missing_deliveries':300-len(seen),'duplicates':duplicates,
   'p50_ms':percentile(samples,.5),'p95_ms':percentile(samples,.95),'p99_ms':percentile(samples,.99),
   'pressure_requests_written':flood_count,'server_cpu_core_percent':cpu,'peak_rss_mib':max(rss,default=0)/2**20,'peak_threads':max(threads,default=0),'duration_s':elapsed}
  if flood:flood.cancel();await asyncio.gather(flood,return_exceptions=True)
  for task in tasks:task.cancel()
  await asyncio.gather(*tasks,return_exceptions=True);tasks=[]
  # Unlike the old throughput harness, exercise actual client teardown here.
  for p in peers:p.w.transport.abort()
  await asyncio.sleep(1)
  metrics=folder/'transport.jsonl'
  result['transport']=[json.loads(line) for line in metrics.read_text().splitlines()] if metrics.exists() else []
  with (folder/'samples.csv').open('w',newline='') as f:
   w=csv.writer(f);w.writerow(['receiver','sequence','delivery_ms']);w.writerows(records)
  (folder/'summary.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
  return result
 finally:
  if flood:flood.cancel()
  for task in tasks:task.cancel()
  await asyncio.gather(*tasks,*([flood] if flood else []),return_exceptions=True)
  await stop(proc,log,peers)

async def main(a):
 out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=True);results=[]
 for pressure in ([False,True] if not a.pressure_only else [True]):
  for repeat in range(1,a.repeats+1):
   for variant in (['baseline','candidate'] if repeat%2 else ['candidate','baseline']):
    build=getattr(a,variant)
    if not build:continue
    r=await experiment(build,out/f'{variant}-pressure{int(pressure)}-r{repeat}',pressure)
    r.update(variant=variant,repeat=repeat);results.append(r)
    (out/'results.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
    print(json.dumps({k:v for k,v in r.items() if k!='transport'}),flush=True)
 if a.check:
  assert all(r['received_deliveries']==300 and not r['duplicates'] for r in results)
  assert all(any(t['stop_reason']==3 for t in r['transport']) for r in results if r['pressure'])
  assert all(len(r['transport'])==8 for r in results), 'all connection writers must finish teardown'
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--baseline');p.add_argument('--candidate',required=True);p.add_argument('--out',required=True);p.add_argument('--repeats',type=int,default=3);p.add_argument('--pressure-only',action='store_true');p.add_argument('--check',action='store_true');asyncio.run(main(p.parse_args()))
