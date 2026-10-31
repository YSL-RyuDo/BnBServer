"""Exercise session reservation, reconnect and framing on the actual loopback server."""
import argparse, asyncio, json
from pathlib import Path
from auth_regression import launch, request
from run import fixture, stop

async def main(a):
    out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False)
    data=out/'data';fixture(data,4)
    proc,log,p=await launch(Path(a.build).resolve(),data)
    results=[]
    def check(name,ok):
        results.append({'case':name,'pass':bool(ok)})
        print(name+': '+('PASS' if ok else 'FAIL'),flush=True)
    try:
        responses=await asyncio.gather(request(p[0],'LOGIN|u0,synthetic-only'),request(p[1],'LOGIN|u0,synthetic-only'))
        check('atomic_duplicate_login',sorted(responses)==['ALREADY_LOGGED_IN|','LOGIN_SUCCESS|u0,n0'])
        winner=0 if responses[0].startswith('LOGIN_SUCCESS|') else 1
        loser=1-winner
        await p[winner].send('QUIT|\n')
        check('quit_closes_session',await asyncio.wait_for(p[winner].r.read(),5)==b'')
        response=''
        for attempt in range(50):
            response=await request(p[loser],'LOGIN|u0,synthetic-only')
            if response.startswith('LOGIN_SUCCESS|'):break
            await asyncio.sleep(.02)
        check('reconnect_after_cleanup',response=='LOGIN_SUCCESS|u0,n0')
        await p[2].send('LOGIN|u1,synthetic-')
        await asyncio.sleep(.03)
        await p[2].send('only\n')
        check('split_auth_frame',await p[2].until('')=='LOGIN_SUCCESS|u1,n1')
        await p[3].send('REGISTER|fresh,synthetic-new,freshnick\nLOGIN|fresh,synthetic-new\n')
        check('coalesced_register_login',await p[3].until('')=='REGISTER_SUCCESS|' and await p[3].until('')=='LOGIN_SUCCESS|fresh,freshnick')
        check('identity_switch_rejected',await request(p[3],'LOGIN|u3,synthetic-only')=='ALREADY_LOGGED_IN|')
    finally:await stop(proc,log,p)
    (out/'results.json').write_text(json.dumps(results,indent=2))
    if not all(r['pass'] for r in results):raise SystemExit(1)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--build',required=True);p.add_argument('--out',required=True)
    asyncio.run(main(p.parse_args()))
