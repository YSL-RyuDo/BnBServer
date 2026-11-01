"""Untrusted result packets must never mutate combat or map state. Synthetic loopback only."""
import argparse, asyncio, json
from pathlib import Path
from run import start,setup,stop

async def main(a):
    b=Path(a.build).resolve();out=Path(a.out).resolve();meta=json.loads((b/'build.json').read_text())
    proc,log,peers=await start(b/'bin/TeamOFYServer.exe',meta['port'],out,4)
    tasks=[];rows=[];results=[];fences=[asyncio.Event() for _ in peers]
    async def collect(p):
        while line:=await p.r.readline():
            s=line.decode().strip()
            if 'ROOM_LIST|' in s:fences[p.i].set()
            if s.startswith(('DAMAGE|','PLAYER_DEAD|','WEAPON_ATTACK|','PLACE_BALLOON_RESULT|','REMOVE_BALLOON|','WATER_SPREAD|','DESTROY','MELODY_','MOVE_RESULT|')):rows.append((p.i,s))
    try:
        await setup(peers);await peers[0].send('START_GAME|r0\n');await peers[0].until('GAME_START|')
        tasks=[asyncio.create_task(collect(p)) for p in peers]
        cases=[('damage_without_server_attack','HIT|0|n0|n1|1'),('water_without_server_water','WATER_HIT|n0|1'),
            ('remote_weapon_origin','WEAPON_ATTACK|n0|0|999,1,999|0'),('remote_balloon_origin','PLACE_BALLOON|n0|999,999|0'),
            ('nonexistent_balloon_explosion','REMOVE_BALLOON|n0|5,5|0'),('arbitrary_block_destruction','DESTROY_BLOCK|Base3_1_1'),
            ('arbitrary_wall_destruction','HITWALL|Base4_1_1'),('arbitrary_projectile_destruction','DESTROY_SPELL|n1_Spell'),
            ('unregistered_melody','MELODY_SPAWN|n0|999,1,999|0'),('projectile_teleport','MELODY_MOVE|n0|999,1,999|0'),
            ('player_teleport','MOVE|n0|999,999')]
        for name,packet in cases:
            rows.clear()
            for e in fences:e.clear()
            await peers[0].send(packet+'\nGET_ROOM_LIST|\n');await asyncio.wait_for(fences[0].wait(),5)
            for p in peers[1:]:await p.send('GET_ROOM_LIST|\n')
            await asyncio.wait_for(asyncio.gather(*(e.wait() for e in fences)),5)
            results.append({'case':name,'pass':not rows,'unauthorized_deliveries':len(rows)});print(results[-1],flush=True)
        (out/'results.json').write_text(json.dumps(results,indent=2));assert all(r['pass'] for r in results)
    finally:
        for t in tasks:t.cancel()
        await asyncio.gather(*tasks,return_exceptions=True);await stop(proc,log,peers)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--build',required=True);p.add_argument('--out',required=True)
    asyncio.run(main(p.parse_args()))
