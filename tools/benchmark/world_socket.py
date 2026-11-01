"""Server movement, registered attack and timed water without client outcome messages."""
import argparse,asyncio,json,math
from pathlib import Path
from run import start,setup,stop
from world_client import WorldClient

async def main(a):
    b=Path(a.build).resolve();out=Path(a.out).resolve();meta=json.loads((b/'build.json').read_text())
    proc,log,peers=await start(b/'bin/TeamOFYServer.exe',meta['port'],out,8);client=None;results=[]
    def check(name,ok):results.append({'case':name,'pass':bool(ok)});print(results[-1],flush=True)
    try:
        await setup(peers);client=WorldClient(peers)
        await client.request(0,'CHOOSE_CHARACTER|r0|n0|1','UPDATE_CHARACTER|')
        line=await client.request(0,'START_GAME|r0','GAME_START|');match=int(line.split('|')[2])
        first=await client.request(0,'GET_MAP|r0|Map1','MAP_DATA|');second=await client.request(0,'GET_MAP|r0|Map1','MAP_DATA|')
        check('stable_spawn_snapshot',first==second)
        positions={e.split(':')[0]:(int(e.split(':')[1]),int(e.split(':')[2])-13) for e in first.split('|')[3].split(',')}
        for i in range(3):await peers[i].send(f'MATCH_READY|{match}\n')
        await peers[0].send(f'ATTACK|{match}|1|90\n');await asyncio.sleep(.12)
        check('no_attack_before_all_clients_ready',not any(s.startswith('WEAPON_SPAWN|') for _,s in client.rows))
        await peers[3].send(f'MATCH_READY|{match}\n');await client.wait(lambda:sum(s==f'COMBAT_BEGIN|{match}' for _,s in client.rows)==4)
        p=positions['n0'];direction=-1 if p[0]==2 else 1
        await peers[0].send(f'MOVE_INPUT|{match}|1|{direction},0\n');await asyncio.sleep(.12);await peers[0].send(f'MOVE_INPUT|{match}|2|0,0\n');await asyncio.sleep(.08)
        actual=client.latest('n0',positions);distance=math.dist(actual,p)
        check('server_moves_from_direction_at_bounded_speed',.15<distance<.6)
        await peers[0].send(f'MOVE_INPUT|{match}|1|{-direction},0\n');await asyncio.sleep(.12)
        check('stale_input_cannot_restart_movement',math.dist(client.latest('n0',positions),actual)<.001)
        offset=len(client.rows);await peers[0].send('ATTACK|999|9|90\n');await asyncio.sleep(.08)
        check('stale_match_attack_rejected',not any(s.startswith('WEAPON_SPAWN|') for _,s in client.rows[offset:]))
        target=min(range(1,4),key=lambda i:math.dist(actual,positions[f'n{i}']))
        await client.fire(0,match,1,target,positions)
        await client.wait(lambda:sum(s.startswith(f'DAMAGE|n{target}|10|90|') for _,s in client.rows)==4)
        check('registered_arrow_causes_server_damage',True)
        offset=len(client.rows);await client.fire(0,match,2,target,positions);await asyncio.sleep(.1)
        check('server_attack_cooldown',not any(s.startswith('WEAPON_SPAWN|') for _,s in client.rows[offset:]))
        offset=len(client.rows);await peers[2].send(f'BALLOON|{match}|1\n')
        await client.wait(lambda:sum(s.startswith('PLACE_BALLOON_RESULT|n2|') for _,s in client.rows[offset:])==4)
        expected=f"{positions['n2'][0]},{positions['n2'][1]}"
        check('balloon_uses_server_position_and_equipped_type',all(s==f'PLACE_BALLOON_RESULT|n2|{expected}|0' for _,s in client.rows[offset:] if s.startswith('PLACE_BALLOON_RESULT|')))
        await asyncio.sleep(2.7);check('balloon_not_removed_early',not any(s.startswith('REMOVE_BALLOON|') for _,s in client.rows[offset:]))
        await client.wait(lambda:sum(s.startswith('WATER_SPREAD|n2|') for _,s in client.rows[offset:])==4)
        await client.wait(lambda:sum(s.startswith('DAMAGE|n2|10|') for _,s in client.rows[offset:])==4)
        check('silent_client_still_receives_water_damage',True)
        check('world_events_stay_in_room',not any(i>=4 and s.startswith(('DAMAGE|','WEAPON_','WATER_SPREAD|','PLAYER_STATE|')) for i,s in client.rows))
        (out/'results.json').write_text(json.dumps(results,indent=2));assert all(r['pass'] for r in results)
    finally:
        if client:await client.close()
        await stop(proc,log,peers)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--build',required=True);p.add_argument('--out',required=True);asyncio.run(main(p.parse_args()))
