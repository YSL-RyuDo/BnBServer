"""Cooperative lethal races and recoverable reward-write failure, using synthetic data."""
import argparse, asyncio, csv, json
from pathlib import Path
import run
from run import start, stop
original_fixture=run.fixture
def fixture(folder,n):
    original_fixture(folder,n)
    (folder/'CharStat.csv').write_text('CharIndex,Health,Attack\n'+''.join(f'{i},10,10\n' for i in range(7)))
run.fixture=fixture

async def main(a):
    b=Path(a.build).resolve();out=Path(a.out).resolve();meta=json.loads((b/'build.json').read_text())
    proc,log,peers=await start(b/'bin/TeamOFYServer.exe',meta['port'],out,4)
    tasks=[];rows=[];results=[];fences=[asyncio.Event() for _ in peers]
    async def collect(p):
        while line:=await p.r.readline():
            s=line.decode().strip()
            if s.startswith('ROOM_LIST|'):fences[p.i].set()
            else:rows.append((p.i,s))
    async def drain():
        for e in fences:e.clear()
        for p in peers:await p.send('GET_ROOM_LIST|\n')
        await asyncio.wait_for(asyncio.gather(*(e.wait() for e in fences)),5)
        # Each sender has completed; fence observers again to drain cross-thread delivery.
        for e in fences:e.clear()
        for p in peers:await p.send('GET_ROOM_LIST|\n')
        await asyncio.wait_for(asyncio.gather(*(e.wait() for e in fences)),5)
    def check(name,ok):
        results.append({'case':name,'pass':bool(ok)});print(results[-1],flush=True)
    def count(prefix):return sum(s.startswith(prefix) for _,s in rows)
    def balances():
        with (out/'UserProfile.csv').open() as f:return {r['id']:int(r['money0']) for r in csv.DictReader(f)}
    try:
        for p in peers:
            await p.send(f'LOGIN|u{p.i},synthetic-only\n');await p.until('LOGIN_SUCCESS|')
            if p.i==0:await p.send('CREATE_ROOM|coop|Map1||1\n');await p.until('CREATE_ROOM_SUCCESS|')
            else:await p.send('ENTER_ROOM|coop||1\n');await p.until('ENTER_ROOM_SUCCESS|')
        await peers[0].send('START_GAME|coop\n');await peers[0].until('GAME_START|')
        tasks=[asyncio.create_task(collect(p)) for p in peers];await drain();rows.clear()
        await peers[0].send('HIT|0|n0|n2|1\n');await drain()
        check('friendly_fire_rejected',count('DAMAGE|')==0);rows.clear()
        await asyncio.gather(peers[0].send('HIT|0|n0|n1|1\n'),peers[2].send('HIT|0|n2|n1|1\n'))
        await drain();check('concurrent_lethal_hit_once',count('DAMAGE|n1|10|0|1|1')==4 and count('PLAYER_DEAD|n1|1')==4 and count('REWARD_RESULT')==0);rows.clear()
        await peers[1].send('HIT|0|n1|n0|1\n');await drain();check('dead_attacker_rejected',count('DAMAGE|')==0);rows.clear()
        # Block only the journal rename, before its commit point, in this test directory.
        blocker=out/'users.redo';blocker.mkdir()
        await peers[0].send('HIT|0|n0|n3|1\n');await drain()
        check('reward_failure_keeps_money_unchanged',count('FILE_WRITE_ERROR|')==1 and count('REWARD_RESULT')==0 and all(v==100000 for v in balances().values()));rows.clear()
        await asyncio.gather(*(p.send(f'READY_TO_EXIT|n{p.i}\n') for p in peers));await drain()
        check('no_exit_before_reward_commit',count('GAME_END')==0 and count('READY_TO_EXIT')==0);rows.clear()
        blocker.rmdir()
        await peers[3].send('DEAD|n3\n');await drain()
        check('zero_hp_receipt_retries_reward_once',count('TEAM_WIN|Blue,n0,n2')==4 and count('REWARD_RESULT')==4 and balances()=={'u0':100500,'u1':100200,'u2':100500,'u3':100200});rows.clear()
        await peers[3].send('DEAD|n3\nDEAD|n3\n');await peers[0].send('HIT|0|n0|n2|1\n');await drain()
        check('resolved_match_cannot_damage_or_reward_again',count('DAMAGE|')==0 and count('REWARD_RESULT')==0);rows.clear()
        await peers[0].send('READY_TO_EXIT|n2\n');await drain()
        check('ready_owner_spoof_rejected',count('READY_TO_EXIT')==0);rows.clear()
        await asyncio.gather(*(p.send(f'READY_TO_EXIT|n{p.i}\nREADY_TO_EXIT|n{p.i}\n') for p in peers));await drain()
        check('one_end_after_all_members_ready',count('GAME_END')==4)
        (out/'results.json').write_text(json.dumps(results,indent=2));assert all(r['pass'] for r in results)
    finally:
        for t in tasks:t.cancel()
        await asyncio.gather(*tasks,return_exceptions=True);await stop(proc,log,peers)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--build',required=True);p.add_argument('--out',required=True)
    asyncio.run(main(p.parse_args()))
