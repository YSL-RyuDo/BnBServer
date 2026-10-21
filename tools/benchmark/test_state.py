from pathlib import Path
import subprocess,json,argparse
from build import build,build_command
from run import fixture

def main(out):
 repo=Path(__file__).resolve().parents[2];out=Path(out).resolve();build(repo,out,39099)
 (out/'source/main.cpp').write_bytes((repo/'tools/benchmark/state_tests.cpp').read_bytes())
 with (out/'test-build.log').open('wb') as log:subprocess.run(build_command(out),stdout=log,stderr=subprocess.STDOUT,check=True)
 data=out/'data';fixture(data,4)
 # Only the first character is owned, so duplicate purchase exercises a real transition.
 p=data/'UserCharacters.csv';p.write_text('id,char0,char1,char2,char3,char4,char5,char6\n'+''.join(f'u{i},1,0,0,0,0,0,0\n' for i in range(4)))
 exe=out/'bin/TeamOFYServer.exe';results=[]
 for mode,code in [('',0),('crash',73),('recover',0)]:
  r=subprocess.run([str(exe)]+([mode] if mode else []),cwd=data,capture_output=True,timeout=90)
  (out/('results-'+(mode or 'state')+'.txt')).write_bytes(r.stdout+r.stderr)
  results.append({'case':mode or 'state','exit':r.returncode,'pass':r.returncode==code});print(results[-1],flush=True)
 (out/'results.json').write_text(json.dumps(results,indent=2));assert all(x['pass'] for x in results)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);main(p.parse_args().out)
