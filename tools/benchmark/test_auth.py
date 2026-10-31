"""Build and exercise actual CNG/UserManager/CsvStore with only synthetic data."""
from pathlib import Path
import argparse, json, subprocess
from build import build, build_command
from run import fixture

def main(out):
    repo=Path(__file__).resolve().parents[2];out=Path(out).resolve()
    out.mkdir(parents=True,exist_ok=False)
    build(repo,out,39109)
    (out/'source/main.cpp').write_bytes((repo/'tools/benchmark/auth_tests.cpp').read_bytes())
    with (out/'test-build.log').open('wb') as log:
        subprocess.run(build_command(out),stdout=log,stderr=subprocess.STDOUT,check=True)
    data=out/'data';fixture(data,4)
    results=[]
    for mode,code in [('',0),('crash',73),('recover',0)]:
        run=subprocess.run([str(out/'bin/TeamOFYServer.exe')]+([mode] if mode else []),cwd=data,capture_output=True,timeout=90)
        (out/('results-'+(mode or 'auth')+'.txt')).write_bytes(run.stdout+run.stderr)
        results.append({'case':mode or 'auth','exit':run.returncode,'pass':run.returncode==code})
        print(results[-1],flush=True)
    (out/'results.json').write_text(json.dumps(results,indent=2))
    if not all(r['pass'] for r in results): raise SystemExit(1)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--out',required=True);main(p.parse_args().out)
