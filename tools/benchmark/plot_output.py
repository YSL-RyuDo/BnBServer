"""Render delivery counts. Missing responses never become zero-latency samples."""
import argparse,json
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

def main(a):
 rows=json.loads(Path(a.results).read_text()); out=Path(a.out);out.mkdir(parents=True,exist_ok=True)
 for lang,labels in {'en':['Room A reader stalled: successful responses inside room B','Responses / expected 300','Before','After','Run'], 'ko':['A방 수신 중단 시, 정상 B방 내부 응답 성공 수','응답 수 / 기대값 300','수정 전','수정 후','회차'], 'ja':['A室の受信停止時、正常なB室内の応答成功数','応答数 / 期待値300','変更前','変更後','回']}.items():
  plt.rcParams.update({'font.family':'MS Gothic' if lang=='ja' else 'Malgun Gothic','svg.fonttype':'none','axes.unicode_minus':False,'font.size':11})
  fig,ax=plt.subplots(figsize=(9,4.3));x=list(range(3))
  for v,offset,color,label in [('baseline',-.19,'#9b9385',labels[2]),('candidate',.19,'#4d654b',labels[3])]:
   values=[next(r['received_deliveries'] for r in rows if r['pressure'] and r['variant']==v and r['repeat']==i+1) for i in x]
   bars=ax.bar([i+offset for i in x],values,width=.36,color=color,label=label)
   for bar,n in zip(bars,values):ax.text(bar.get_x()+bar.get_width()/2,n+5,str(n),ha='center',va='bottom')
  ax.set_ylim(0,380);ax.set_xticks(x,[f'{labels[4]} {i+1}' for i in x]);ax.set_ylabel(labels[1]);ax.set_title(labels[0],loc='left',pad=18)
  ax.spines[['top','right']].set_visible(False);ax.legend(loc='upper right',frameon=False);fig.tight_layout()
  p=out/f'output-delivery-{lang}.svg';fig.savefig(p);plt.close(fig)
  p.write_text('\n'.join(s.rstrip() for s in p.read_text(encoding='utf-8').splitlines())+'\n',encoding='utf-8')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--results',required=True);p.add_argument('--out',required=True);main(p.parse_args())
