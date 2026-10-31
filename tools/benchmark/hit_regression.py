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
 b=Path(a.build).resolve();out=Path(a.out).resolve();meta=json.loads((b/'build.json').read_text())
 proc,log,peers=await start(b/'bin/TeamOFYServer.exe',meta['port'],out,28);rows=[];tasks=[]
 async def read(p):
  while line:=await p.r.readline():
   if line.startswith(b'DAMAGE|'):rows.append((p.i,line.decode().strip()))
 try:
  await setup(peers)
  for index in range(7):
   p=peers[index*4]
   await p.send(f'CHOOSE_CHARACTER|r{index}|n{p.i}|{index}\n');await p.until('UPDATE_CHARACTER|')
   await p.send(f'START_GAME|r{index}\n');await p.until('GAME_START|')
  tasks=[asyncio.create_task(read(p)) for p in peers]
  for index in range(7):await peers[index*4].send(f'HIT|{index}|n{index*4}|n{index*4+1}|{index+1}\n')
  async def wait():
   while len(rows)<28:await asyncio.sleep(.01)
  await asyncio.wait_for(wait(),5)
  wanted=sorted((i,f'DAMAGE|n{k*4+1}|{10+k}|{90-k}|{k+1}|1') for k in range(7) for i in range(k*4,k*4+4))
  result={'pass':sorted(rows)==wanted,'weapons':7,'expected':28,'received':len(rows)}
  (out/'results.json').write_text(json.dumps(result,indent=2));print(result);assert result['pass']
 finally:
  for t in tasks:t.cancel()
  await asyncio.gather(*tasks,return_exceptions=True);await stop(proc,log,peers)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--build',required=True);p.add_argument('--out',required=True);asyncio.run(main(p.parse_args()))
