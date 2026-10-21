"""Alternate variant order at each load and repetition to reduce ordering bias."""
import asyncio, argparse, json, platform, sys, time, subprocess
from pathlib import Path
import psutil
from run import measure

async def main(a):
    out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=True)
    builds={k:Path(getattr(a,k)).resolve() for k in ['baseline','candidate']}
    cpu=subprocess.check_output(['powershell','-NoProfile','-Command','(Get-CimInstance Win32_Processor).Name'],text=True).strip()
    manifest={'date':time.strftime('%Y-%m-%d'),'os':platform.platform(),'python':sys.version,'psutil':psutil.__version__,'cpu':cpu,'logical_cpus':psutil.cpu_count(),'physical_cpus':psutil.cpu_count(logical=False),'ram_gib':round(psutil.virtual_memory().total/2**30,2),
      'variants':{k:json.loads((v/'build.json').read_text()) for k,v in builds.items()},
      'method':{'clients':a.clients,'rooms':'4 clients per room','repeats':a.repeats,'warmup_s':2,'measurement_s':10,'drain_s':2,'offered_hz_per_client':20,'schedule':'all senders scheduled every 50ms, alternating variant order per repeat','network':'same Windows host, TCP IPv4 loopback; no Unity renderer','tcp_nodelay':'client: asyncio default on; server: unchanged default','cpu_percent':'100% = one logical core','latency':'sender pre-write to observer newline receipt; includes client event loop and TCP scheduling','quantiles':'nearest rank, per repetition; publication uses median of run quantiles','workload_scope':'MOVE relay in populated waiting rooms; no battle simulation, DB or shop traffic'}}
    (out/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8');results=[]
    for n in a.clients:
        for rep in range(1,a.repeats+1):
            order=['baseline','candidate'] if rep%2 else ['candidate','baseline']
            for variant in order:
                b=builds[variant];m=manifest['variants'][variant]
                r=await measure(b/'bin/TeamOFYServer.exe',m['port'],out/f'{variant}-c{n}-r{rep}',n,20,10,2)
                r.update(variant=variant,repeat=rep);results.append(r)
                (out/'results.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
                print(json.dumps(r),flush=True)
                await asyncio.sleep(.3)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--baseline',required=True);p.add_argument('--candidate',required=True);p.add_argument('--out',required=True);p.add_argument('--clients',type=int,nargs='+',default=[4,16,32,64]);p.add_argument('--repeats',type=int,default=3);asyncio.run(main(p.parse_args()))
