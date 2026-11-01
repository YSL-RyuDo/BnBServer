"""Cooperative lethal races and recoverable reward-write failure, using synthetic data."""
import argparse, asyncio, csv, json
from pathlib import Path
import run
from run import start, stop
from world_client import WorldClient
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
        bootstrap=WorldClient(peers)
        for p in peers:await bootstrap.request(p.i,f'CHOOSE_CHARACTER|coop|n{p.i}|1','UPDATE_CHARACTER|')
        match,positions=await bootstrap.begin(0,'coop',range(4))
        await bootstrap.close()
        async def fire(sender,seq,target):
            import math
            origin=positions[f'n{sender}'];dest=positions[f'n{target}']
            yaw=math.degrees(math.atan2(dest[0]-origin[0],dest[1]-origin[1]))%360
            await peers[sender].send(f'ATTACK|{match}|{seq}|{yaw}\n')
        async def wait_for(predicate):
            async def poll():
                while not predicate():await asyncio.sleep(.01)
            await asyncio.wait_for(poll(),5)
        tasks=[asyncio.create_task(collect(p)) for p in peers];await drain();rows.clear()
        await fire(0,1,2);await asyncio.sleep(.3);await drain()
        check('friendly_fire_rejected',count('DAMAGE|')==0);rows.clear()
        await asyncio.sleep(1.25)
        await asyncio.gather(fire(0,2,1),fire(2,1,1))
        await wait_for(lambda:count('PLAYER_DEAD|n1|1')==4)
        await drain();check('concurrent_lethal_hit_once',count('DAMAGE|n1|10|0|1|1')==4 and count('PLAYER_DEAD|n1|1')==4 and count('REWARD_RESULT')==0);rows.clear()
        await fire(1,1,0);await asyncio.sleep(.1);await drain();check('dead_attacker_rejected',count('DAMAGE|')==0);rows.clear()
        # Block only the journal rename, before its commit point, in this test directory.
        blocker=out/'users.redo';blocker.mkdir()
        await asyncio.sleep(1.5);await fire(0,3,3)
        await wait_for(lambda:b'Combat reward save failed' in (out/'server.log').read_bytes());await drain()
        check('reward_failure_keeps_money_unchanged',count('REWARD_RESULT')==0 and all(v==100000 for v in balances().values()));rows.clear()
        await asyncio.gather(*(p.send(f'READY_TO_EXIT|n{p.i}\n') for p in peers));await drain()
        check('no_exit_before_reward_commit',count('GAME_END')==0 and count('READY_TO_EXIT')==0);rows.clear()
        blocker.rmdir()
        await peers[3].send('DEAD|n3\n');await wait_for(lambda:count('REWARD_RESULT')==4);await drain()
        check('zero_hp_receipt_retries_reward_once',count('TEAM_WIN|Blue,n0,n2')==4 and count('REWARD_RESULT')==4 and balances()=={'u0':100500,'u1':100200,'u2':100500,'u3':100200});rows.clear()
        await peers[3].send('DEAD|n3\nDEAD|n3\n');await fire(0,1,2);await asyncio.sleep(.3);await drain()
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
