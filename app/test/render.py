import sys
from PIL import Image, ImageDraw, ImageFont
ops=open(sys.argv[1],encoding='utf-8').read().split('\n')
outdir=sys.argv[2]
def rgb(c): return ((c>>11)*255//31,((c>>5)&63)*255//63,(c&31)*255//31)
img=Image.new('RGB',(320,240),(0,0,0)); d=ImageDraw.Draw(img)
fs=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf',11)
fl=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf',15)
n=0
for l in ops:
    if not l: continue
    p=l.split(' ',6)
    if p[0]=='R':
        x,y,w,h,c=map(int,p[1:6]); d.rectangle([x,y,x+w-1,y+h-1],fill=rgb(c))
    elif p[0]=='T':
        x,y,lg,f,b=map(int,p[1:6]); s=l.split(' ',6)[6]
        cw,ch=(10,18) if lg else (7,14)
        if not s: continue
        d.rectangle([x,y,x+len(s)*cw-1,y+ch-1],fill=rgb(b))
        for i,chx in enumerate(s):
            d.text((x+i*cw,y+(1 if not lg else 0)),chx,font=fl if lg else fs,fill=rgb(f))
    elif p[0]=='S':
        img.resize((640,480),Image.NEAREST).save(f'{outdir}/{p[1]}.png'); n+=1
print(n,'captures')
