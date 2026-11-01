"""Combat authorization regression after moving outcomes into the simulation."""
import argparse,asyncio,json,math
from pathlib import Path
from run import start,setup,stop
from world_client import WorldClient

async def main(a):
 b=Path(a.build).resolve();out=Path(a.out).resolve();meta=json.loads((b/'build.json').read_text())
 proc,log,peers=await start(b/'bin/TeamOFYServer.exe',meta['port'],out,8);client=None;results=[]
 async def reject(name,sender,packet,prefixes=('DAMAGE|','PLAYER_DEAD|','REWARD_RESULT','GAME_END','WEAPON_SPAWN|')):
  offset=len(client.rows);await peers[sender].send(packet+'\n');await client.fence();await asyncio.sleep(.06)
  ok=not any(s.startswith(prefixes) for _,s in client.rows[offset:]);results.append({'case':name,'pass':ok});print(results[-1],flush=True)
 try:
  await setup(peers);client=WorldClient(peers)
  await reject('attack_before_match',0,'ATTACK|1|1|0')
  await reject('water_cross_room_spoof',0,'WATER_HIT|n4|999999')
  await reject('water_negative_damage',0,'WATER_HIT|n0|-999999')
  await reject('character_owner_spoof',0,'CHOOSE_CHARACTER|r0|n1|6',('UPDATE_CHARACTER|',))
  await reject('weapon_visual_spoof',0,'WEAPON_ATTACK|n1|0|0,0,0|0',('WEAPON_ATTACK|','WEAPON_SPAWN|'))
  await client.request(0,'CHOOSE_CHARACTER|r0|n0|1','UPDATE_CHARACTER|')
  match,positions=await client.begin(0,'r0',range(4));results.append({'case':'start_match','pass':True})
  await reject('selection_locked_during_match',0,'CHOOSE_CHARACTER|r0|n0|6',('UPDATE_CHARACTER|',))
  await reject('ready_before_resolution',0,'READY_TO_EXIT|n0',('READY_TO_EXIT|','GAME_END'))
  await reject('forged_death',0,'DEAD|n1')
  await reject('self_death_while_alive',0,'DEAD|n0')
  await reject('client_selected_weapon_rejected',0,'HIT|6|n0|n1|1')
  await reject('stale_match',0,'ATTACK|999|1|0')
  target=min(range(1,4),key=lambda i:math.dist(positions['n0'],positions[f'n{i}']))
  await client.fire(0,match,1,target,positions)
  await client.wait(lambda:sum(s.startswith(f'DAMAGE|n{target}|10|90|1|1') for _,s in client.rows)==4)
  results.append({'case':'valid_registered_attack_absolute_hp','pass':True})
  await reject('server_attack_cooldown',0,f'ATTACK|{match}|2|0')
  await reject('water_without_collision',2,'WATER_HIT|n2|1')
  await reject('water_other_player',0,'WATER_HIT|n3|1')
  await reject('trailing_fields',0,f'ATTACK|{match}|3|0|extra')
  await asyncio.sleep(1.55);await client.fire(0,match,3,target,positions)
  await client.wait(lambda:sum(s.startswith(f'DAMAGE|n{target}|10|80|1|2') for _,s in client.rows)==4)
  results.append({'case':'cooldown_expires','pass':True})
  await reject('exit_room_spoof',0,'EXIT_ROOM|r0|n1',('ROOM_USER_LIST|','EXIT_ROOM|','ROOM_EXIT|'))
  (out/'results.json').write_text(json.dumps(results,indent=2));assert all(r['pass'] for r in results)
 finally:
  if client:await client.close()
  await stop(proc,log,peers)

if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--build',required=True);p.add_argument('--out',required=True);asyncio.run(main(p.parse_args()))
