"""Compile actual UserManager sources, replace only executable main with lookup harness."""
import argparse,subprocess,json,hashlib,shutil
from pathlib import Path
def prepare(build):
 build=Path(build).resolve();src=build/'source';meta=json.loads((build/'build.json').read_text())
 harness=Path(__file__).with_suffix('.cpp').read_bytes()
 if b'LoadAttackStats' in (src/'UserManager.h').read_bytes():harness=b'#define CACHED_ATTACK_STATS\n'+harness
 (src/'main.cpp').write_bytes(harness)
 cmd=meta['build_command']
 with (build/'lookup-build.log').open('wb') as out:subprocess.run(cmd,stdout=out,stderr=subprocess.STDOUT,check=True)
 (build/'lookup-build.json').write_text(json.dumps({'original_build':meta,'replacement':'main.cpp only, stats_lookup.cpp harness; CACHED_ATTACK_STATS for candidate','compiled_source_hashes':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in src.iterdir() if p.suffix in ['.cpp','.h','.vcxproj']},'binary_sha256':hashlib.sha256((build/'bin/TeamOFYServer.exe').read_bytes()).hexdigest()},indent=2),encoding='utf-8')
 return build/'bin/TeamOFYServer.exe'
def fixture(folder):
 folder.mkdir(parents=True,exist_ok=True)
 (folder/'CharStat.csv').write_text('CharIndex,Health,Attack\n'+''.join(f'{i},100,{10+i}\n' for i in range(7)),encoding='ascii')
def main(a):
 out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=True)
 exes={v:prepare(getattr(a,v)) for v in ['baseline','candidate']};rs=[]
 for rep in range(1,6):
  for v in (['baseline','candidate'] if rep%2 else ['candidate','baseline']):
   folder=out/f'{v}-{rep}';fixture(folder)
   p=subprocess.run([str(exes[v])],cwd=folder,capture_output=True,check=True,timeout=120)
   lines=p.stdout.decode(errors='replace').splitlines();r=json.loads(next(x for x in lines if x.startswith('{')))
   assert r['checksum']==sum(10+i%7 for i in range(100000));r.update(variant=v,repeat=rep);rs.append(r);print(json.dumps(r),flush=True)
 (out/'results.json').write_text(json.dumps(rs,indent=2),encoding='utf-8')
 folder=out/'checks';fixture(folder)
 p=subprocess.run([str(exes['candidate']),'--test'],cwd=folder,capture_output=True,check=True,timeout=20)
 (out/'checks.txt').write_bytes(p.stdout);print(p.stdout.decode(errors='replace'),flush=True)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--baseline');p.add_argument('--candidate',required=True);p.add_argument('--out',required=True);p.add_argument('--check-only',action='store_true');a=p.parse_args()
 if a.check_only:
  exe=prepare(a.candidate);folder=Path(a.out).resolve();fixture(folder)
  result=subprocess.run([str(exe),'--test'],cwd=folder,capture_output=True,check=True,timeout=20);(folder/'checks.txt').write_bytes(result.stdout);print(result.stdout.decode(errors='replace'))
 else:
  if not a.baseline:p.error('--baseline is required for measurement')
  main(a)
