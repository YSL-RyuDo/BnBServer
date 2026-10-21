"""Build the real SocketOutput class with deterministic partial-write and queue-limit tests."""
from pathlib import Path
import subprocess,argparse,xml.etree.ElementTree as ET
def main(out):
 repo=Path(__file__).resolve().parents[2];out=Path(out).resolve();out.mkdir(parents=True,exist_ok=True)
 ns='http://schemas.microsoft.com/developer/msbuild/2003';tag=lambda x:'{'+ns+'}'+x
 ET.register_namespace('',ns);tree=ET.parse(repo/'TeamOFYServer/TeamOFYServer.vcxproj');root=tree.getroot()
 for group in root.findall(tag('ItemGroup')):
  if group.find(tag('ClCompile')) is not None or group.find(tag('ClInclude')) is not None:root.remove(group)
 group=ET.SubElement(root,tag('ItemGroup'))
 for p in [repo/'TeamOFYServer/SocketOutput.cpp',repo/'tools/benchmark/output_tests.cpp']:ET.SubElement(group,tag('ClCompile'),Include=str(p))
 for group in root.findall(tag('ItemDefinitionGroup')):
  compiler=group.find(tag('ClCompile'));ET.SubElement(compiler,tag('AdditionalIncludeDirectories')).text=str(repo/'TeamOFYServer')+';%(AdditionalIncludeDirectories)'
 project=out/'TransportTests.vcxproj';tree.write(project,encoding='utf-8',xml_declaration=True)
 vswhere='C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe'
 msbuild=subprocess.check_output([vswhere,'-latest','-requires','Microsoft.Component.MSBuild','-find','MSBuild/**/Bin/MSBuild.exe'],text=True).strip().splitlines()[0]
 with (out/'build.log').open('wb') as log:subprocess.run([msbuild,str(project),'/p:Configuration=Release','/p:Platform=x64',f'/p:OutDir={out}/bin/',f'/p:IntDir={out}/obj/','/v:minimal','/nologo'],stdout=log,stderr=subprocess.STDOUT,check=True)
 result=subprocess.run([str(out/'bin/TransportTests.exe')],capture_output=True,timeout=20)
 (out/'results.txt').write_bytes(result.stdout+result.stderr);print(result.stdout.decode());result.check_returncode()
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--out',required=True);main(p.parse_args().out)
