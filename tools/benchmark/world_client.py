"""Synthetic socket client for the intent protocol; waits for observable server events."""
import asyncio, math

class WorldClient:
    def __init__(self,peers):
        self.peers=peers;self.rows=[];self.tasks=[asyncio.create_task(self.read(p)) for p in peers]
    async def read(self,p):
        while line:=await p.r.readline():self.rows.append((p.i,line.decode().strip()))
    async def wait(self,predicate,timeout=5):
        async def poll():
            while not predicate():await asyncio.sleep(.01)
        await asyncio.wait_for(poll(),timeout)
    async def request(self,sender,packet,prefix):
        offset=len(self.rows);await self.peers[sender].send(packet+'\n')
        await self.wait(lambda:any(i==sender and s.startswith(prefix) for i,s in self.rows[offset:]))
        return next(s for i,s in self.rows[offset:] if i==sender and s.startswith(prefix))
    async def fence(self):
        await asyncio.gather(*(self.request(p.i,'GET_ROOM_LIST|','ROOM_LIST|') for p in self.peers))
    async def begin(self,leader,room,members):
        start=await self.request(leader,'START_GAME|'+room,'GAME_START|');match=int(start.split('|')[2]);positions={}
        for i in members:
            data=await self.request(i,f'GET_MAP|{room}|Map1','MAP_DATA|')
            for entry in data.split('|')[3].split(','):
                nick,x,z,weapon=entry.split(':');positions[nick]=(int(x),int(z)-13)
        offset=len(self.rows)
        for i in members:await self.peers[i].send(f'MATCH_READY|{match}\n')
        await self.wait(lambda:sum(s==f'COMBAT_BEGIN|{match}' for _,s in self.rows[offset:])==len(members))
        return match,positions
    def latest(self,nick,fallback):
        for _,s in reversed(self.rows):
            f=s.split('|')
            if len(f)==5 and f[0]=='PLAYER_STATE' and f[2]==nick:return tuple(map(float,f[3].split(',')))
        return fallback[nick]
    async def fire(self,sender,match,sequence,target,positions):
        a=self.latest(f'n{sender}',positions);b=self.latest(f'n{target}',positions)
        yaw=math.degrees(math.atan2(b[0]-a[0],b[1]-a[1]))%360
        await self.peers[sender].send(f'ATTACK|{match}|{sequence}|{yaw}\n')
    async def close(self):
        for t in self.tasks:t.cancel()
        await asyncio.gather(*self.tasks,return_exceptions=True)
