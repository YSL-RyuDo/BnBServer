"""Same black-box security assertions before/after; synthetic data, loopback only.
Never print response bodies or credential material, including on assertion failure.
"""
import argparse, asyncio, csv, hashlib, json, subprocess
from pathlib import Path
from run import fixture, Peer, stop

async def launch(build, folder, count=4):
    meta = json.loads((build/'build.json').read_text())
    log = (folder/'server.log').open('ab')
    proc = subprocess.Popen([str(build/'bin/TeamOFYServer.exe')], cwd=folder,
                            stdout=log, stderr=subprocess.STDOUT,
                            creationflags=subprocess.CREATE_NO_WINDOW)
    peers = []
    try:
        for attempt in range(600):
            if proc.poll() is not None: raise RuntimeError('startup rejected')
            try:
                r,w = await asyncio.open_connection('127.0.0.1', meta['port'])
                break
            except OSError: await asyncio.sleep(.05)
        else: raise TimeoutError('listen')
        peers.append(Peer(0,r,w))
        for i in range(1,count):
            r,w = await asyncio.open_connection('127.0.0.1', meta['port'])
            peers.append(Peer(i,r,w))
        return proc,log,peers
    except:
        await stop(proc,log,peers)
        raise

async def request(peer, message):
    await peer.send(message+'\n')
    return await peer.until('', timeout=15)

async def main(a):
    build, out = Path(a.build).resolve(), Path(a.out).resolve()
    # Refuse reusing a data directory; every invocation has a fresh synthetic fixture.
    out.mkdir(parents=True, exist_ok=False)
    data=out/'data'; fixture(data,4)
    original=(data/'UsersAccount.csv').read_bytes()
    results=[]
    def check(name, ok):
        results.append({'case':name,'pass':bool(ok)})
        print(name+': '+('PASS' if ok else 'FAIL'),flush=True)
    proc,log,p=await launch(build,data)
    try:
        check('legacy_login', (await request(p[0],'LOGIN|u0,synthetic-only')).startswith('LOGIN_SUCCESS|'))
        check('wrong_password',await request(p[1],'LOGIN|u1,incorrect-secret')=='WRONG_PASSWORD|')
        check('duplicate_login',await request(p[1],'LOGIN|u0,synthetic-only')=='ALREADY_LOGGED_IN|')
        response=await request(p[1],'LOGIN|u1,synthetic-only')
        check('password_free_response', response=='LOGIN_SUCCESS|u1,n1')
        check('register',await request(p[2],'REGISTER|newuser,new-secret-123,newnick')=='REGISTER_SUCCESS|')
        check('duplicate_id',await request(p[2],'REGISTER|newuser,new-secret-123,othernick')=='DUPLICATE_ID|')
        check('duplicate_nickname',await request(p[2],'REGISTER|otheruser,new-secret-123,newnick')=='DUPLICATE_NICK|')
        check('empty_password',await request(p[2],'REGISTER|emptyuser,,emptynick')=='EMPTY_PASSWORD|')
        check('new_login',(await request(p[2],'LOGIN|newuser,new-secret-123')).startswith('LOGIN_SUCCESS|'))
        check('same_connection_identity_preserved',await request(p[0],'LOGIN|u2,synthetic-only')=='ALREADY_LOGGED_IN|')
        check('invalid_register',await request(p[3],'REGISTER|bad,new-secret-123,nick,extra')=='REGISTER_ERROR|')
        check('unknown_id',await request(p[3],'LOGIN|missing,synthetic-only')=='ID_NOT_FOUND|')
        check('legacy_source_unchanged',(data/'UsersAccount.csv').read_bytes()==original)
        v2=data/'UsersAccount.v2.csv'
        records=list(csv.DictReader(v2.open(encoding='utf-8'))) if v2.exists() else []
        valid=len(records)==5 and all('password_hash' in r for r in records)
        salts=[]
        if valid:
            for r in records:
                fields=r['password_hash'].split('$')
                if len(fields)!=5 or fields[:3]!=['','pbkdf2-sha256','600000']:
                    valid=False; break
                salt=bytes.fromhex(fields[3]); digest=bytes.fromhex(fields[4]); salts.append(salt)
                pw=b'new-secret-123' if r['id']=='newuser' else b'synthetic-only'
                valid &= len(salt)==16 and len(digest)==32 and digest==hashlib.pbkdf2_hmac('sha256',pw,salt,600000)
        check('hashed_storage_interoperability',valid)
        check('independent_salts',valid and len(set(salts))==len(salts))
    finally: await stop(proc,log,p)
    logs=(data/'server.log').read_bytes()
    check('credential_free_logs',all(s not in logs for s in [b'synthetic-only',b'incorrect-secret',b'new-secret-123',b'LOGIN|',b'REGISTER|',b'$pbkdf2-sha256$']))
    proc,log,p=await launch(build,data)
    try:
        check('restart_legacy_login',(await request(p[0],'LOGIN|u0,synthetic-only')).startswith('LOGIN_SUCCESS|'))
        check('restart_new_login',(await request(p[1],'LOGIN|newuser,new-secret-123')).startswith('LOGIN_SUCCESS|'))
    finally: await stop(proc,log,p)
    (out/'results.json').write_text(json.dumps(results,indent=2))
    raise SystemExit(0 if all(x['pass'] for x in results) else 1)

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--build',required=True);parser.add_argument('--out',required=True)
    asyncio.run(main(parser.parse_args()))
