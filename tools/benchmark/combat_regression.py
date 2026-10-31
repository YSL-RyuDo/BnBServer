"""Real socket combat authority checks; only synthetic accounts in an isolated directory."""
import argparse, asyncio, json
from pathlib import Path
from run import start, setup, stop

async def main(a):
    build=Path(a.build).resolve(); out=Path(a.out).resolve()
    meta=json.loads((build/'build.json').read_text())
    proc,log,peers=await start(build/'bin/TeamOFYServer.exe',meta['port'],out,8)
    tasks=[]; rows=[]; results=[]; fences=[asyncio.Event() for _ in peers]
    async def collect(p):
        while line:=await p.r.readline():
            s=line.decode().strip()
            if s.startswith('ROOM_LIST|'):fences[p.i].set()
            else:rows.append((p.i,s))
    async def exercise(name,sender,packet,check):
        rows.clear()
        for e in fences:e.clear()
        await peers[sender].send(packet+'\nGET_ROOM_LIST|\n')
        await asyncio.wait_for(fences[sender].wait(),5)
        for p in peers:
            if p.i!=sender:await p.send('GET_ROOM_LIST|\n')
        await asyncio.wait_for(asyncio.gather(*(e.wait() for e in fences)),5)
        result={'case':name,'pass':bool(check(rows))}
        results.append(result);print(result,flush=True)
    def no_combat(rows):
        return not any(s.startswith(('DAMAGE|','PLAYER_HIT|','PLAYER_DEAD|','REWARD_RESULT','GAME_END')) for _,s in rows)
    def damage(nick,hp,revision):
        expected=f'DAMAGE|{nick}|10|{hp}|1|{revision}'
        return lambda rows:sorted((i,s) for i,s in rows if s.startswith(('DAMAGE|','PLAYER_HIT|')))==[(i,expected) for i in range(4)]
    try:
        await setup(peers);tasks=[asyncio.create_task(collect(p)) for p in peers]
        await exercise('hit_before_match',0,'HIT|0|n0|n1|1',no_combat)
        await exercise('water_cross_room_spoof',0,'WATER_HIT|n4|999999',no_combat)
        await exercise('water_negative_damage',0,'WATER_HIT|n0|-999999',no_combat)
        await exercise('character_owner_spoof',0,'CHOOSE_CHARACTER|r0|n1|6',lambda r:not any(s.startswith('UPDATE_CHARACTER|') for _,s in r))
        await exercise('weapon_visual_spoof',0,'WEAPON_ATTACK|n1|0|0,0,0|0',lambda r:not any(s.startswith('WEAPON_ATTACK|') for _,s in r))
        await exercise('start_match',0,'START_GAME|r0',lambda r:any(s.startswith('GAME_START|r0') for _,s in r))
        await exercise('selection_locked_during_match',0,'CHOOSE_CHARACTER|r0|n0|6',lambda r:not any(s.startswith('UPDATE_CHARACTER|') for _,s in r))
        await exercise('ready_before_resolution',0,'READY_TO_EXIT|n0',lambda r:not any(s.startswith('READY_TO_EXIT') for _,s in r))
        await exercise('forged_death',0,'DEAD|n1',no_combat)
        await exercise('self_death_while_alive',0,'DEAD|n0',no_combat)
        await exercise('wrong_weapon',0,'HIT|6|n0|n1|1',no_combat)
        await exercise('stale_match',0,'HIT|0|n0|n1|999',no_combat)
        await exercise('valid_hit_absolute_hp',0,'HIT|0|n0|n1|1',damage('n1',90,1))
        await exercise('shared_victim_cooldown',2,'HIT|0|n2|n1|1',no_combat)
        await exercise('water_uses_server_damage',2,'WATER_HIT|n2|1',damage('n2',90,2))
        await exercise('water_other_player',0,'WATER_HIT|n3|1',no_combat)
        await exercise('trailing_fields',0,'HIT|0|n0|n3|1|garbage',no_combat)
        await asyncio.sleep(1.05)
        await exercise('cooldown_expires',0,'HIT|0|n0|n1|1',damage('n1',80,3))
        await exercise('exit_room_spoof',0,'EXIT_ROOM|r0|n1',lambda r:not r)
        (out/'results.json').write_text(json.dumps(results,indent=2))
        if not all(r['pass'] for r in results):raise SystemExit(1)
    finally:
        for t in tasks:t.cancel()
        await asyncio.gather(*tasks,return_exceptions=True)
        await stop(proc,log,peers)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--build',required=True);p.add_argument('--out',required=True)
    asyncio.run(main(p.parse_args()))
