"""Render measured run quantiles. Error bars are min/max across three runs, not confidence intervals."""
import argparse,json,statistics
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

def render(data,out):
 rs=json.loads(Path(data).read_text(encoding='utf-8'));out=Path(out);out.mkdir(parents=True,exist_ok=True)
 plt.rcParams.update({'font.family':['Malgun Gothic','DejaVu Sans'],'font.size':12,'svg.fonttype':'path','axes.spines.top':False,'axes.spines.right':False,'figure.facecolor':'white'})
 labels={
 'en':('Movement relay latency','p95 delivery time (ms, log scale)','Connected clients, 4 per room','Original','October fix','Median of 3 runs; whiskers = min/max. Same-host loopback, 20 Hz/client.','Received application payload','MOVE_RESULT payload (MiB per run)','Includes unintended cross-room deliveries in the original.'),
 'ko':('이동 메시지 전달 지연','p95 전달 시간 (ms, 로그 축)','연결 수, 방마다 4명','기존 구현','10월 수정','3회 중앙값, 수염은 최솟값과 최댓값. 같은 PC, 클라이언트당 20 Hz.','수신한 애플리케이션 데이터','MOVE_RESULT 데이터 (MiB / 회)','기존 구현의 다른 방으로 잘못 보낸 데이터도 포함합니다.'),
 'ja':('移動メッセージの通知遅延','p95 通知時間 (ms、対数軸)','接続数、1ルーム4人','従来の実装','10月の修正','3回の中央値、エラーバーは最小値と最大値。同一PC、20 Hz/接続。','受信したアプリケーションデータ','MOVE_RESULT データ (MiB / 回)','従来実装で他ルームに誤送信したデータも含みます。')}
 for lang,t in labels.items():
  plt.rcParams['font.family']=['MS Gothic','DejaVu Sans'] if lang=='ja' else ['Malgun Gothic','DejaVu Sans']
  for name,key,title,ylabel,scale in [('latency','p95_ms',t[0],t[1],1),('payload','application_bytes_received',t[6],t[7],2**20)]:
   fig,ax=plt.subplots(figsize=(9,4.9));x=np.arange(4)
   for i,(variant,label,color,hatch) in enumerate([('baseline',t[3],'#aea89c','//'),('candidate',t[4],'#423e36','')]):
    groups=[[r[key]/scale for r in rs if r['variant']==variant and r['clients']==n] for n in [4,16,32,64]]
    med=np.array([statistics.median(g) for g in groups]);low=np.array([min(g) for g in groups]);high=np.array([max(g) for g in groups])
    ax.bar(x+(i-.5)*.34,med,.30,label=label,color=color,hatch=hatch,yerr=[med-low,high-med],capsize=4)
   if name=='latency':ax.set_yscale('log');ax.set_ylim(1,20000)
   ax.set_xticks(x,['4','16','32','64']);ax.set_xlabel(t[2]);ax.set_ylabel(ylabel);ax.set_title(title,loc='left',pad=20,fontweight='bold');ax.legend(frameon=False,loc='upper left')
   ax.grid(axis='y',alpha=.18);ax.set_axisbelow(True)
   fig.text(.02,.025,t[5] if name=='latency' else t[8],fontsize=10,color='#514d45')
   fig.tight_layout(rect=(0,.07,1,1));fig.savefig(out/f'{name}-{lang}.svg',metadata={'Date':None});plt.close(fig)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--results',required=True);p.add_argument('--out',required=True);a=p.parse_args();render(a.results,a.out)
