import asyncio,argparse,json
from pathlib import Path
from run import start,setup,stop
async def main(a):
 b=Path(a.build).resolve();out=Path(a.out).resolve();meta=json.loads((b/'build.json').read_text())
 proc,log,peers=await start(b/'bin/TeamOFYServer.exe',meta['port'],out,8);rows=[];tasks=[]
 async def read(p):
  try:
   while line:=await p.r.readline():
    if line.startswith(b'DAMAGE|'):rows.append((p.i,line.decode().strip()))
  except OSError:pass
 try:
  await setup(peers);tasks=[asyncio.create_task(read(p)) for p in peers]
  for index in range(7):await peers[0].send(f'HIT|{index}|n0|n1\n')
  await asyncio.sleep(.5)
  result={'pass':len(rows)==28 and all(i<4 and s=='DAMAGE|n1|10' for i,s in rows),'expected':28,'received':len(rows),'cross_room':sum(i>=4 for i,_ in rows)}
  (out/'results.json').write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result));assert result['pass']
 finally:
  for t in tasks:t.cancel()
  await asyncio.gather(*tasks,return_exceptions=True);await stop(proc,log,peers)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--build',required=True);p.add_argument('--out',required=True);asyncio.run(main(p.parse_args()))
