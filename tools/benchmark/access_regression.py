"""Authentication boundary checks against the real server, synthetic loopback data only."""
import argparse, asyncio, json
from pathlib import Path
from run import Peer, start, setup, stop
from world_client import WorldClient
import math

async def main(a):
    build=Path(a.build).resolve();out=Path(a.out).resolve()
    meta=json.loads((build/'build.json').read_text())
    proc,log,peers=await start(build/'bin/TeamOFYServer.exe',meta['port'],out,8)
    tasks=[];results=[];damage=[];messages=[];ready=[]
    try:
        await setup(peers)
        bootstrap=WorldClient(peers)
        await bootstrap.request(0,'CHOOSE_CHARACTER|r0|n0|1','UPDATE_CHARACTER|')
        match,positions=await bootstrap.begin(0,'r0',range(4))
        await bootstrap.close()
        target=min(range(1,4),key=lambda i:math.dist(positions['n0'],positions[f'n{i}']))
        delta=(positions[f'n{target}'][0]-positions['n0'][0],positions[f'n{target}'][1]-positions['n0'][1])
        yaw=math.degrees(math.atan2(*delta))%360
        r,w=await asyncio.open_connection('127.0.0.1',meta['port'])
        peers.append(Peer(8,r,w))
        ready=[asyncio.Event() for _ in peers]
        async def collect(p):
            while line:=await p.r.readline():
                if line.startswith(b'DAMAGE|'):damage.append((p.i,line.decode().strip()))
                if line.startswith(b'AUTH_REQUIRED|'):messages.append(p.i)
                if line.startswith(b'ROOM_LIST|'):ready[p.i].set()
        tasks=[asyncio.create_task(collect(p)) for p in peers]
        async def exercise(name,sender,packet,expected,requires_auth=False):
            damage.clear();messages.clear()
            for event in ready:event.clear()
            # FIFO fences: sender processes the request before its room-list response;
            # each observer then drains queued deliveries before its own response.
            await peers[sender].send(packet+'\nGET_ROOM_LIST|\n')
            await asyncio.wait_for(ready[sender].wait(),5)
            for p in peers:
                if p.i!=sender:await p.send('GET_ROOM_LIST|\n')
            await asyncio.wait_for(asyncio.gather(*(event.wait() for event in ready)),5)
            if expected:
                async def wait_damage():
                    while len(damage)<4:await asyncio.sleep(.01)
                await asyncio.wait_for(wait_damage(),5)
            wanted=[(i,f'DAMAGE|n{target}|10|90|1|1') for i in range(4)] if expected else []
            passed=sorted(damage)==wanted and (not requires_auth or sender in messages)
            results.append({'case':name,'pass':passed,'damage_deliveries':len(damage)})
            print(results[-1],flush=True)
        await exercise('unauthenticated_hit',8,'HIT|0|n0|n1|1',False,True)
        await exercise('same_room_sender_spoof',1,'HIT|0|n0|n1|1',False)
        await exercise('other_room_sender_spoof',4,'HIT|0|n0|n1|1',False)
        await exercise('cross_room_target',0,'HIT|0|n0|n4|1',False)
        await exercise('missing_target',0,'HIT|0|n0|missing|1',False)
        await exercise('authenticated_attack',0,f'ATTACK|{match}|1|{yaw}',True)
        await exercise('unauthenticated_profile_request',8,'GETINFO|n0',False,True)
        await exercise('unauthenticated_purchase',8,'BUY_CHAR|n0,1',False,True)
        await peers[2].send('LOGOUT|n2\n')
        # Same connection: logout is processed before HIT; old identity must be unusable.
        await exercise('logged_out_hit',2,'HIT|0|n0|n1|1',False,True)
        (out/'results.json').write_text(json.dumps(results,indent=2))
        if not all(x['pass'] for x in results):raise SystemExit(1)
    finally:
        for t in tasks:t.cancel()
        await asyncio.gather(*tasks,return_exceptions=True)
        await stop(proc,log,peers)

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--build',required=True);parser.add_argument('--out',required=True)
    asyncio.run(main(parser.parse_args()))
