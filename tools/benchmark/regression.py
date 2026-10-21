"""Protocol probes against the real loopback server. Run each case in a fresh process."""
import asyncio, json, argparse
from pathlib import Path
from run import start, setup, stop

async def case(build,out,name):
    meta=json.loads((build/'build.json').read_text());proc,log,peers=await start(build/'bin/TeamOFYServer.exe',meta['port'],out/name,8)
    records=[];tasks=[]
    async def collect(p):
        try:
            while True:
                line=await p.r.readline()
                if not line:return
                if line.startswith(b'MOVE_RESULT|'):records.append((p.i,line.decode().strip()))
        except (OSError,ValueError):pass
    try:
        await setup(peers);tasks=[asyncio.create_task(collect(p)) for p in peers]
        if name=='room_isolation':await peers[0].send('MOVE|n0|10,0\n')
        elif name=='split_frame':
            await peers[0].send('MOVE|n0|12');await asyncio.sleep(.12);await peers[0].send(',0\n')
        elif name=='coalesced_frames':await peers[0].send('MOVE|n0|10,0\nMOVE|n0|11,0\nMOVE|n0|12,0\n')
        elif name=='complete_and_partial':
            await peers[0].send('MOVE|n0|10,0\nMOVE|n0|11');await asyncio.sleep(.12);await peers[0].send(',0\n')
        elif name=='spoofed_nickname':await peers[0].send('MOVE|n1|10,0\n')
        elif name=='invalid_coordinate':await peers[0].send('MOVE|n0|not-a-number,0\n')
        elif name=='nonfinite_coordinate':await peers[0].send('MOVE|n0|nan,0\n')
        elif name=='oversized_frame':
            await peers[0].send('X'*5000);await asyncio.sleep(.15)
        await asyncio.sleep(.35)
        health_count = None
        if name in ('invalid_coordinate','nonfinite_coordinate'):
            # A process waiting in crash reporting can still appear alive.
            try: await peers[1].send('MOVE|n1|999,0\n')
            except OSError: pass
            await asyncio.sleep(.35)
            health_count=sum(',999.000000,' in line for _,line in records)
            records[:]=[(i,line) for i,line in records if ',999.000000,' not in line]
        alive=proc.poll() is None
        expected={'room_isolation':3,'split_frame':3,'coalesced_frames':9,'complete_and_partial':6}.get(name,0)
        cross=sum(i>=4 for i,_ in records)
        expected_x={'room_isolation':[10],'split_frame':[12],'coalesced_frames':[10,11,12],'complete_and_partial':[10,11]}.get(name,[])
        actual=sorted((i,int(float(s.split('|')[1].split(',')[1]))) for i,s in records if 'nan' not in s.lower())
        wanted=sorted((i,x) for i in [1,2,3] for x in expected_x)
        closed=peers[0].r.at_eof()
        passed=alive and len(records)==expected and cross==0 and actual==wanted
        if name=='oversized_frame':passed=passed and closed
        if health_count is not None:passed=passed and health_count==3
        return {'case':name,'pass':passed,'server_alive':alive,'deliveries':len(records),'cross_room_deliveries':cross,'peer_closed':closed,'expected_deliveries':expected,'subsequent_valid_deliveries':health_count}
    finally:
        for t in tasks:t.cancel()
        await asyncio.gather(*tasks,return_exceptions=True);await stop(proc,log,peers)

async def main(a):
    out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=True);results=[]
    for name in ['room_isolation','split_frame','coalesced_frames','complete_and_partial','spoofed_nickname','invalid_coordinate','nonfinite_coordinate','oversized_frame']:
        r=await case(Path(a.build).resolve(),out,name);results.append(r);print(json.dumps(r),flush=True)
    (out/'results.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--build',required=True);p.add_argument('--out',required=True);asyncio.run(main(p.parse_args()))
