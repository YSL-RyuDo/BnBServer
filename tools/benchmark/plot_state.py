"""Plot every run and medians from compare.py JSON; no pooled quantiles."""
import argparse,json,statistics
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
p=argparse.ArgumentParser();p.add_argument('--results',required=True);p.add_argument('--out',required=True)
a=p.parse_args();rows=json.loads(Path(a.results).read_text(encoding='utf-8'));out=Path(a.out);out.mkdir(parents=True,exist_ok=True)
metrics=['p95_ms','server_cpu_core_percent']
for lang in ["en","ko","ja"]:
 plt.rcParams['font.family']='MS Gothic' if lang=='ja' else 'Malgun Gothic' if lang=='ko' else 'DejaVu Sans'
 fig,axes=plt.subplots(1,2,figsize=(11,3.6),layout='constrained')
 labels={'en':['Before','After','Connections','p95 (ms)','CPU (% of one core)'],'ko':['수정 전','수정 후','연결 수','p95 (ms)','CPU (논리 코어 하나 기준 %)'],'ja':['変更前','変更後','接続数','p95 (ms)','CPU (論理コア1個基準 %)']}[lang]
 for ax,k,ylabel in zip(axes,metrics[:2],labels[3:]):
  for i,v in enumerate(['baseline','candidate']):
   color=['#948b7c','#4e604b'][i]
   for j,n in enumerate([8,32,64]):
    vals=[x[k] for x in rows if x['clients']==n and x['variant']==v]
    x=j+(-.13 if i==0 else .13)
    ax.scatter([x+(q-2)*.018 for q in range(5)],vals,s=22,color=color,alpha=.65,label=labels[i] if j==0 else None)
    ax.plot([x-.08,x+.08],[statistics.median(vals)]*2,color=color,lw=3)
  ax.set_xticks([0,1,2],[8,32,64]);ax.set_xlabel(labels[2]);ax.set_ylabel(ylabel);ax.set_ylim(bottom=0);ax.spines[['top','right']].set_visible(False);ax.grid(axis='y',alpha=.18);ax.legend(frameon=False)
 fig.savefig(out/f'state-retest-{lang}.svg');plt.close(fig)

