"""Compile production collision simulation and exercise a deterministic clock plus shipped maps."""
import argparse, subprocess, shutil
from pathlib import Path
from build import build,build_command

def main(out):
    repo=Path(__file__).resolve().parents[2];out=Path(out).resolve();build(repo,out,39131)
    (out/'source/main.cpp').write_bytes((repo/'tools/benchmark/collision_tests.cpp').read_bytes())
    with (out/'unit-build.log').open('wb') as log:subprocess.run(build_command(out),stdout=log,stderr=subprocess.STDOUT,check=True)
    data=out/'data';data.mkdir(exist_ok=True)
    for name in ['Map1.csv','Map2.csv']:shutil.copy2(repo/'TeamOFYServer'/name,data/name)
    p=subprocess.run([str(out/'bin/TeamOFYServer.exe')],cwd=data,capture_output=True,timeout=60)
    (out/'results.txt').write_bytes(p.stdout+p.stderr);print(p.stdout.decode(errors='replace'));print(p.stderr.decode(errors='replace'));p.check_returncode()

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--out',required=True);main(p.parse_args().out)
