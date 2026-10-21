"""Plot all three MOVE repetitions; do not hide latency tradeoffs."""
import argparse,json,statistics as st
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
def main(a):
 rows=json.loads(Path(a.results).read_text());out=Path(a.out);out.mkdir(parents=True,exist_ok=True)
 for lang,labels in {'en':['Observed peak threads','Server CPU, one core = 100%','Connections','Per-connection writer','Shared IOCP worker','Dots: each run; bars: median of 3 runs'], 'ko':['관찰한 최대 스레드 수','서버 CPU, 코어 하나 = 100%','연결 수','연결별 송신 스레드','공용 IOCP 워커','점: 각 회차, 막대: 3회 중앙값'], 'ja':['観測した最大スレッド数','サーバーCPU, 1コア = 100%','接続数','接続別送信スレッド','共通IOCPワーカー','点: 各実行、棒: 3回の中央値']}.items():
  plt.rcParams.update({'font.family':'MS Gothic' if lang=='ja' else 'Malgun Gothic','svg.fonttype':'none','axes.unicode_minus':False,'font.size':11})
  fig,axes=plt.subplots(1,2,figsize=(11,4.6))
  for ax,k,title in zip(axes,['server_peak_threads','server_cpu_core_percent'],labels[:2]):
   for variant,offset,color,label in [('baseline',-.2,'#9b9385',labels[3]),('candidate',.2,'#4d654b',labels[4])]:
    vals=[[r[k] for r in rows if r['clients']==n and r['variant']==variant] for n in [8,32,64]];med=[st.median(v) for v in vals]
    ax.bar([i+offset for i in range(3)],med,.36,color=color,label=label)
    for i,v in enumerate(vals):
     ax.scatter([i+offset-.065+j*.065 for j in range(3)],v,s=17,color='#29251d',zorder=3)
     ax.annotate(f'{med[i]:.0f}' if k.endswith('threads') else f'{med[i]:.2f}',(i+offset,max(v)),xytext=(0,8),textcoords='offset points',ha='center',fontsize=10)
   ax.set_title(title,loc='left',pad=18);ax.set_xticks(range(3),['8','32','64']);ax.set_xlabel(labels[2]);ax.spines[['top','right']].set_visible(False);ax.set_ylim(0,ax.get_ylim()[1]*1.16)
  handles,names=axes[0].get_legend_handles_labels();fig.legend(handles,names,loc='upper center',ncol=2,bbox_to_anchor=(.5,1),frameon=False);fig.text(.5,.015,labels[5],ha='center',fontsize=10);fig.tight_layout(rect=(0,.05,1,.9))
  fig.savefig(out/f'iocp-cost-{lang}.svg');fig.savefig(out/f'iocp-cost-{lang}.png',dpi=130);plt.close(fig)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--results',required=True);p.add_argument('--out',required=True);main(p.parse_args())
