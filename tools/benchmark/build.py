"""Build an isolated loopback-only copy; leave repository data and binaries untouched."""
import argparse, hashlib, json, shutil, subprocess
from pathlib import Path

def build(repo, destination, port, ref=None):
    repo, destination = Path(repo).resolve(), Path(destination).resolve()
    destination.mkdir(parents=True, exist_ok=True)
    source = destination / 'source'
    source.mkdir(exist_ok=True)
    hashes = {}
    for f in (repo/'TeamOFYServer').iterdir():
        if f.suffix not in ('.h','.cpp','.vcxproj'): continue
        data=subprocess.check_output(['git','-C',str(repo),'show',f'{ref}:TeamOFYServer/{f.name}']) if ref else f.read_bytes()
        hashes[f.name]=hashlib.sha256(data).hexdigest()
        if f.name=='main.cpp':
            assert data.count(b'Initialize(9000)')==1
            data=data.replace(b'Initialize(9000)',f'Initialize({port})'.encode())
        if f.name=='Server.cpp':
            assert data.count(b'= INADDR_ANY;')==1
            data=data.replace(b'= INADDR_ANY;',b'= htonl(INADDR_LOOPBACK);')
        (source/f.name).write_bytes(data)
    vswhere=Path('C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe')
    msbuild=subprocess.check_output([str(vswhere),'-latest','-requires','Microsoft.Component.MSBuild','-find','MSBuild/**/Bin/MSBuild.exe'],text=True).strip().splitlines()[0]
    cmd=[msbuild,str(source/'TeamOFYServer.vcxproj'),'/p:Configuration=Release','/p:Platform=x64',f'/p:OutDir={destination / "bin"}\\',f'/p:IntDir={destination / "obj"}\\','/v:minimal','/nologo']
    with (destination/'build.log').open('wb') as log: subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
    exe=destination/'bin/TeamOFYServer.exe'
    manifest={'source_commit':subprocess.check_output(['git','-C',str(repo),'rev-parse',ref or 'HEAD'],text=True).strip(),
      'source_hashes':hashes,'binary_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'port':port,
      'adapter':['main.cpp: test port instead of 9000','Server.cpp: bind loopback instead of all interfaces'],
      'configuration':'Release x64, MSVC v143, C++14','build_command':cmd}
    (destination/'build.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print(exe,flush=True)
    return exe

if __name__=='__main__':
    a=argparse.ArgumentParser();a.add_argument('--out',required=True);a.add_argument('--port',type=int,default=39091)
    a.add_argument('--ref',help='Build committed sources from this Git revision using the current harness')
    args=a.parse_args();build(Path(__file__).resolve().parents[2],args.out,args.port,args.ref)
