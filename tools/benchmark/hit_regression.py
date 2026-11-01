"""All seven selected weapons use cached server damage and stay within their room."""
import asyncio,argparse,json
from pathlib import Path
import run
from run import start,setup,stop
original_fixture=run.fixture
def fixture(folder,n):
 original_fixture(folder,n)
 (folder/'CharStat.csv').write_text('CharIndex,Health,Attack\n'+''.join(f'{i},100,{10+i}\n' for i in range(7)))
run.fixture=fixture
async def main(a):
 from world_client import WorldClient
 import math,collections
 b=Path(a.build).resolve();out=Path(a.out).resolve();meta=json.loads((b/'build.json').read_text())
 proc,log,peers=await start(b/'bin/TeamOFYServer.exe',meta['port'],out,28);client=None
 try:
  await setup(peers);client=WorldClient(peers)
  for index in range(7):
   sender=index*4
   await client.request(sender,f'CHOOSE_CHARACTER|r{index}|n{sender}|{index}','UPDATE_CHARACTER|')
   match,positions=await client.begin(sender,f'r{index}',range(sender,sender+4))
   target=min(range(sender+1,sender+4),key=lambda i:math.dist(positions[f'n{sender}'],positions[f'n{i}']))
   await client.fire(sender,match,1,target,positions)
   await client.wait(lambda:sum(s.startswith('DAMAGE|') and int(s.split('|')[4])==match for _,s in client.rows)>=4)
  await asyncio.sleep(.4);await client.fence()
  rows=[(i,s) for i,s in client.rows if s.startswith('DAMAGE|')];groups=collections.defaultdict(list)
  for i,s in rows:groups[s].append(i)
  passed=bool(groups) and len({int(s.split('|')[4]) for s in groups})==7
  for packet,observers in groups.items():
   _,target,damage,hp,match,revision=packet.split('|');room=int(match)-1
   passed &= sorted(observers)==list(range(room*4,room*4+4)) and int(target[1:])//4==room and int(damage)==10+room and int(hp)==90-room
  result={'pass':passed,'weapons':7,'damage_events':len(groups),'deliveries':len(rows),'all_groups_delivered_once_to_four_room_members':passed}
  (out/'results.json').write_text(json.dumps(result,indent=2));print(result);assert passed
 finally:
  if client:await client.close()
  await stop(proc,log,peers)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--build',required=True);p.add_argument('--out',required=True);asyncio.run(main(p.parse_args()))
