"""Real TCP checks for numeric exceptions, logout and connection churn."""
import argparse, asyncio, json
from pathlib import Path
import psutil
from run import start, setup, stop, Peer

async def check(build, out, name):
    meta = json.loads((build/'build.json').read_text())
    proc, log, peers = await start(build/'bin/TeamOFYServer.exe', meta['port'], out/name, 8)
    tasks = []; deliveries = []; errors = []
    async def collect(p):
        try:
            while line := await p.r.readline():
                if line.startswith(b'MOVE_RESULT|n4,'): deliveries.append((p.i, line))
        except OSError: pass
    try:
        await setup(peers)
        tasks = [asyncio.create_task(collect(p)) for p in peers[1:]]
        before = psutil.Process(proc.pid).num_threads()
        if name.startswith('invalid_'):
            value = 'not-a-number' if name == 'invalid_integer' else '9'*50
            await peers[0].send(f'BUY_ICON|n0,{value}\n')
            async def eof():
                while await peers[0].r.readline(): pass
            await asyncio.wait_for(eof(), 3)
        elif name == 'logout_relogin':
            await peers[0].send('LOGOUT|\n')
            await peers[0].until('LOGOUT_SUCCESS|')
            await peers[0].send('LOGIN|u0,synthetic-only\n')
            await peers[0].until('LOGIN_SUCCESS|')
            await peers[0].send('ENTER_ROOM|r0||0\n')
            await peers[0].until('ENTER_ROOM_SUCCESS|')
        else:
            # Reuse the same account 20 times while another room sends MOVE.
            # Each close must remove the login session and release both threads.
            for i in range(20):
                await peers[4].send(f'MOVE|n4|{i},0\n')
                if i % 2 == 0: await peers[0].send('QUIT|\n')
                else: peers[0].w.transport.abort()
                if i % 2 == 0:
                    async def eof():
                        while await peers[0].r.readline(): pass
                    await asyncio.wait_for(eof(), 3)
                peers[0].w.close()
                await asyncio.sleep(.06)
                r, w = await asyncio.open_connection('127.0.0.1', meta['port'])
                peers[0] = Peer(0, r, w)
                await peers[0].send('LOGIN|u0,synthetic-only\n')
                await peers[0].until('LOGIN_SUCCESS|')
                await peers[0].send('ENTER_ROOM|r0||0\n')
                await peers[0].until('ENTER_ROOM_SUCCESS|')
        await peers[4].send('MOVE|n4|999,0\n')
        await asyncio.sleep(.5)
        final = sum(b',999.000000,' in line for _, line in deliveries)
        after = psutil.Process(proc.pid).num_threads()
        expected = 63 if name == 'quit_eof_churn' else 3
        passed = final == 3 and len(deliveries) == expected and all(i in (5,6,7) for i,_ in deliveries)
        if name == 'quit_eof_churn': passed = passed and after <= before
        return dict(case=name, passed=passed, deliveries=len(deliveries), expected=expected,
                    healthy_room_final_deliveries=final, threads_before=before, threads_after=after)
    except (OSError, asyncio.TimeoutError, psutil.Error) as e:
        return dict(case=name, passed=False, error=type(e).__name__)
    finally:
        for t in tasks: t.cancel()
        await asyncio.gather(*tasks, return_exceptions=True)
        await stop(proc, log, peers)

async def main(a):
    build=Path(a.build).resolve(); out=Path(a.out).resolve(); out.mkdir(parents=True, exist_ok=True)
    results=[]
    for name in ['invalid_integer','invalid_integer_range','logout_relogin','quit_eof_churn']:
        result=await check(build,out,name); results.append(result); print(json.dumps(result),flush=True)
    (out/'results.json').write_text(json.dumps(results,indent=2),encoding='utf-8')
    if not all(r['passed'] for r in results): raise SystemExit(1)

if __name__ == '__main__':
    p=argparse.ArgumentParser(); p.add_argument('--build',required=True);p.add_argument('--out',required=True)
    asyncio.run(main(p.parse_args()))
