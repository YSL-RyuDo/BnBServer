"""Real-server regression: listen succeeds six seconds before request processing.
The delay is injected only into an isolated test build, never production sources.
"""
import argparse, hashlib, json, subprocess, sys
from pathlib import Path
from build import build, build_command

def main(out):
    repo=Path(__file__).resolve().parents[2];out=Path(out).resolve()
    out.mkdir(parents=True,exist_ok=False)
    build(repo,out,39114)
    main=out/'source/main.cpp';source=main.read_bytes()
    assert source.count(b'server.Run();')==1
    main.write_bytes(source.replace(b'server.Run();',b'Sleep(6000); server.Run();'))
    with (out/'delayed-build.log').open('wb') as log:
        subprocess.run(build_command(out),stdout=log,stderr=subprocess.STDOUT,check=True)
    meta=json.loads((out/'build.json').read_text())
    meta['adapter'].append('main.cpp: 6000 ms delay after listen, before account initialization')
    meta['binary_sha256']=hashlib.sha256((out/'bin/TeamOFYServer.exe').read_bytes()).hexdigest()
    (out/'build.json').write_text(json.dumps(meta,indent=2))
    result=subprocess.run([sys.executable,str(repo/'tools/benchmark/purchase_regression.py'),
        '--build',str(out),'--out',str(out/'purchase')],capture_output=True,timeout=120)
    (out/'results.txt').write_bytes(result.stdout+result.stderr)
    print(result.stdout.decode(errors='replace'),flush=True)
    result.check_returncode()
    print('PASS delayed startup followed by 32-account purchase regression',flush=True)

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--out',required=True)
    main(parser.parse_args().out)
