import asyncio,json,argparse,csv
from pathlib import Path
import run
original_fixture=run.fixture
def fixture(path,n):
 original_fixture(path,n)
 for p in path.glob("Store*.csv"):
  p.write_text(p.read_text().replace(",5000,0",",5000,COIN"))
 for name,count in [('UserCharacters',7),('UserBalloon',10),('UserEmotes',36),('UserIcon',10)]:
  p=path/(name+'.csv');header=p.read_text().splitlines()[0]
  p.write_text(header+'\n'+''.join(f'u{i},'+','.join(['1']+['0']*(count-1))+'\n' for i in range(n)))
run.fixture=fixture
async def main(a):
 b=Path(a.build).resolve();out=Path(a.out).resolve();m=json.loads((b/'build.json').read_text())
 proc,log,peers=await run.start(b/'bin/TeamOFYServer.exe',m['port'],out,32);tasks=[];lines=[]
 async def collect(p):
  try:
   while line:=await p.r.readline():lines.append((p.i,line.decode(errors='replace').strip()))
  except OSError:pass
 try:
  for p in peers:
   await p.send(f'LOGIN|u{p.i},synthetic-only\n');await p.until('LOGIN_SUCCESS|')
  tasks=[asyncio.create_task(collect(p)) for p in peers]
  commands=['CHAR','BALLOON','EMO','ICON']
  await asyncio.gather(*(p.send((f'BUY_{commands[p.i%4]}|n{p.i},1\n')*2) for p in peers))
  await asyncio.sleep(2)
  results={'server_alive':proc.poll() is None};correct=0
  try:
   profiles={r['id']:r for r in csv.DictReader((out/'UserProfile.csv').open())}
   tables=[{r['id']:r for r in csv.DictReader((out/(name+'.csv')).open())} for name in ['UserCharacters','UserBalloon','UserEmotes','UserIcon']]
   for i in range(32):
    field=['char1','balloon1','emo1','icon1'][i%4]
    if int(profiles[f'u{i}']['money0'])==95000 and int(tables[i%4][f'u{i}'][field])==1:correct+=1
  except (KeyError,ValueError,FileNotFoundError):pass
  results.update(correct_wallet_and_inventory=correct,expected=32,passed=correct==32 and results['server_alive'])
  (out/'results.json').write_text(json.dumps(results,indent=2));print(results,flush=True)
  if not a.observe:assert results['passed']
 finally:
  for t in tasks:t.cancel()
  await asyncio.gather(*tasks,return_exceptions=True);await run.stop(proc,log,peers)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--build',required=True);p.add_argument('--out',required=True);p.add_argument('--observe',action='store_true');asyncio.run(main(p.parse_args()))
