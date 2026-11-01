import asyncio,json,argparse
from pathlib import Path
from run import start,setup,stop
from world_client import WorldClient
import run
original_fixture=run.fixture
def low_health_fixture(folder,n):
 original_fixture(folder,n)
 (folder/'CharStat.csv').write_text('CharIndex,Health,Attack\n'+''.join(f'{i},10,10\n' for i in range(7)))
run.fixture=low_health_fixture
async def case(build,out):
 meta=json.loads((build/'build.json').read_text());proc,log,peers=await start(build/'bin/TeamOFYServer.exe',meta['port'],out,8)
 client=None;lines=[]
 try:
  await setup(peers);client=WorldClient(peers);lines=client.rows
  for cycle in range(12):
   match,positions=await client.begin(0,'r0',range(4))
   await client.fire(3,match,1,0,positions)
   await client.wait(lambda:sum(x.startswith('REWARD_RESULT') for _,x in lines)==4*(cycle+1))
   await asyncio.gather(*(p.send(f'DEAD|n{p.i}\nDEAD|n{p.i}\n') for p in peers[:3]))
   await asyncio.gather(*(p.send(f'READY_TO_EXIT|n{p.i}\nREADY_TO_EXIT|n{p.i}\n') for p in peers[:4]),peers[4].send(f'MOVE|n4|{1000+cycle},0\n'))
   await client.wait(lambda:sum(x=='GAME_END' for _,x in lines)==4*(cycle+1))
   await client.wait(lambda:sum(x.startswith('MOVE_RESULT|n4,') for _,x in lines)==3*(cycle+1))
  results={}
  results['one_reward_per_match']=all(sum(i==p and x.startswith('REWARD_RESULT') for i,x in lines)==12 for p in range(4))
  results['one_game_end_per_match']=all(sum(i==p and x=='GAME_END' for i,x in lines)==12 for p in range(4))
  results['other_room_keeps_responding']=sum(x.startswith('MOVE_RESULT|n4,') for _,x in lines)==36
  import csv
  profiles={r['id']:r for r in csv.DictReader((out/'UserProfile.csv').open())}
  results['rewards_saved_once']=all(int(profiles[f'u{i}']['money0'])==100000+(6000 if i==3 else 2400) for i in range(4))
  # Grow and erase the room vector while existing room paths read snapshots.
  await peers[4].send('EXIT_ROOM|r1|n4\n');await asyncio.sleep(.05)
  for cycle in range(30):
   await asyncio.gather(peers[4].send(f'CREATE_ROOM|temporary{cycle}|Map1||0\n'),peers[0].send('HIT|0|n0|n1\n'))
   await asyncio.sleep(.005)
   await peers[4].send(f'EXIT_ROOM|temporary{cycle}|n4\n')
  await peers[0].send('MOVE|n0|9999,0\n');await asyncio.sleep(.25)
  results['room_churn_keeps_server_alive']=proc.poll() is None and sum(',9999.000000,' in x for _,x in lines)==3
  (out/'results.json').write_text(json.dumps(results,indent=2));print(results,flush=True);assert all(results.values())
 finally:
  if client:await client.close()
  await stop(proc,log,peers)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--build',required=True);p.add_argument('--out',required=True);a=p.parse_args();asyncio.run(case(Path(a.build).resolve(),Path(a.out).resolve()))
