// NumWorks Userland (app .nwa) - menu, mini-jeux, outils. Un seul fichier C, sans libc.
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#include <stdint.h>
#include <stdbool.h>
#include "eadk.h"

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Userland";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64; typedef int32_t i32;
#define RGB(r,g,b) ((u16)((((r)>>3)<<11)|(((g)>>2)<<5)|((b)>>3)))
#define YEL RGB(255,183,52)
#define BLK RGB(0,0,0)
#define WHT RGB(255,255,255)
#define PI_ 3.14159265358979f

// touches (numérotation Ion/EADK)
enum { K_LEFT=0,K_UP=1,K_DOWN=2,K_RIGHT=3,K_OK=4,K_BACK=5,K_HOME=6,K_BKSP=17,K_EXE=52 };
#define KB(k) (1ULL<<(k))

// ---------- utilitaires ----------
static int slen(const char*s){int n=0;while(s[n])n++;return n;}
static int ulen(const char*s){int n=0;for(;*s;s++)if(((u8)*s&0xC0)!=0x80)n++;return n;}
static void scpy(char*d,const char*s){while((*d++=*s++));}
static void sapp(char*d,const char*s){scpy(d+slen(d),s);}
static char* itoa_(i32 v,char*b){char t[12];int n=0;bool neg=v<0;u32 u=neg?(u32)(-(i32)v):(u32)v;
  do{t[n++]='0'+u%10;u/=10;}while(u);int k=0;if(neg)b[k++]='-';while(n)b[k++]=t[--n];b[k]=0;return b;}

// ---------- thème ----------
static u16 C_BG,C_FG,C_MUTED,C_SEL,C_SELTXT;
static int g_dark=0, g_bright=160;
static void theme(void){
  if(!g_dark){C_BG=WHT;C_FG=RGB(36,36,36);C_MUTED=RGB(130,130,130);C_SEL=RGB(36,36,36);C_SELTXT=WHT;}
  else{C_BG=RGB(28,28,32);C_FG=RGB(235,235,235);C_MUTED=RGB(150,150,150);C_SEL=YEL;C_SELTXT=RGB(20,20,20);}
}
static void fill(int x,int y,int w,int h,u16 c){ if(w<=0||h<=0)return; eadk_rect_t r={(u16)x,(u16)y,(u16)w,(u16)h}; eadk_display_push_rect_uniform(r,c);}
static void txt(int x,int y,const char*s,u16 fg,u16 bg,bool large){ eadk_point_t p={(u16)x,(u16)y}; eadk_display_draw_string(s,p,large,fg,bg);}
static void txtc(int cx,int y,const char*s,u16 fg,u16 bg,bool large){ txt(cx-ulen(s)*(large?10:7)/2,y,s,fg,bg,large);}
static void bar(const char*t){ fill(0,0,320,24,BLK); txt(8,3,t,WHT,BLK,true);
  fill(286,7,26,11,WHT);fill(287,8,24,9,BLK);fill(289,10,18,5,YEL);fill(312,10,3,5,WHT);}
static void body(void){ fill(0,24,320,216,C_BG);}
static void hint(const char*h){ fill(0,224,320,16,C_BG); txt(8,225,h,C_MUTED,C_BG,false);}
static void wait(u32 ms){ eadk_timing_msleep(ms);}
static u32 now(void){ return (u32)eadk_timing_millis();}
static u32 rnd(void){ return eadk_random();}

// ---------- clavier avec répétition ----------
static u64 g_prev; static u32 g_rep[5]; static const u8 REPK[5]={0,1,2,3,17};
static u64 poll(void){
  u64 n=eadk_keyboard_scan(); u64 e=n&~g_prev; u32 t=now();
  for(int i=0;i<5;i++){u64 m=KB(REPK[i]); if(e&m)g_rep[i]=t+350; else if((n&m)&&(i32)(t-g_rep[i])>=0){e|=m;g_rep[i]=t+80;}}
  g_prev=n; return e;}
static bool held(int k){ return (g_prev>>k)&1ULL;}

// ---------- menu générique ----------
static int menu(const char*title,const char*const*it,int n,const char*h,int sel){
  int top=0; bool dirty=true;
  for(;;){
    if(dirty){ bar(title); body(); if(sel<top)top=sel; if(sel>=top+8)top=sel-7;
      for(int i=0;i<8&&top+i<n;i++){int y=28+i*24;bool s=(top+i==sel);u16 bg=s?C_SEL:C_BG;
        fill(8,y,304,22,bg); fill(14,y+5,12,12,s?YEL:C_MUTED); txt(34,y+4,it[top+i],s?C_SELTXT:C_FG,bg,false);}
      hint(h); dirty=false;}
    u64 e=poll();
    if(e&KB(K_UP)){sel=(sel+n-1)%n;dirty=true;}
    if(e&KB(K_DOWN)){sel=(sel+1)%n;dirty=true;}
    if(e&(KB(K_OK)|KB(K_EXE)|KB(K_RIGHT)))return sel;
    if(e&(KB(K_BACK)|KB(K_LEFT)))return -1;
    wait(16);
  }}

static void message(const char*title,const char*l1,const char*l2,const char*l3){
  bar(title);body();txt(16,50,l1,C_FG,C_BG,false);if(l2)txt(16,74,l2,C_FG,C_BG,false);if(l3)txt(16,98,l3,C_FG,C_BG,false);
  hint("Retour : quitter");
  for(;;){u64 e=poll();if(e&(KB(K_BACK)|KB(K_OK)|KB(K_LEFT)|KB(K_EXE)))return;wait(16);}}

// ---------- nombres : formatage ----------
static bool isnan_(float v){return v!=v;}
static void fmtd(float v,char*out){
  if(isnan_(v)){scpy(out,"Erreur");return;}
  int k=0; if(v<0){out[k++]='-';v=-v;}
  if(v>1e37f){scpy(out+k,"inf");return;}
  if(v==0){scpy(out+k,"0");return;}
  int e=0; float t=v; while(t>=10.0f){t/=10.0f;e++;} while(t<1.0f){t*=10.0f;e--;}
  float p=1.0f; for(int i=0;i<(e<0?-e:e);i++)p*=10.0f;
  float sc=e>=0?v/p:v*p; u32 m=(u32)(sc*1000000.0f+0.5f); if(m>=10000000u){m/=10;e++;}
  char d[9]; for(int i=6;i>=0;i--){d[i]='0'+m%10;m/=10;} d[7]=0;
  if(e>=-4&&e<7){
    if(e>=0){ for(int i=0;i<=e;i++)out[k++]=i<7?d[i]:'0'; int last=6; while(last>e&&d[last]=='0')last--;
      if(last>e){out[k++]='.';for(int i=e+1;i<=last;i++)out[k++]=d[i];} }
    else{ out[k++]='0';out[k++]='.';for(int i=0;i<-e-1;i++)out[k++]='0'; int last=6;while(last>0&&d[last]=='0')last--; for(int i=0;i<=last;i++)out[k++]=d[i];}
    out[k]=0;
  } else {
    out[k++]=d[0]; int last=6;while(last>0&&d[last]=='0')last--; if(last>0){out[k++]='.';for(int i=1;i<=last;i++)out[k++]=d[i];}
    out[k++]='E'; itoa_(e,out+k);
  }}

// ---------- évaluateur d'expressions ----------
static const char*P; static float VX; static int perr;
static float p_expr(void);
static float sin_(float x){ while(x>PI_)x-=2*PI_; while(x<-PI_)x+=2*PI_; float t=x,s=x; for(int i=1;i<12;i++){t*=-x*x/((2*i)*(2*i+1));s+=t;} return s;}
static float sqrt_(float x){ if(x<0){perr=1;return 0;} if(x==0)return 0; float r=x>1?x:1; for(int i=0;i<60;i++)r=0.5f*(r+x/r); return r;}
static float pw(float b,float e){ int n=(int)e; if((float)n!=e||n>64||n<-64){perr=1;return 0;} float r=1; int m=n<0?-n:n; for(int i=0;i<m;i++)r*=b; return n<0?1/r:r;}
static bool kw(const char*w){int i=0;while(w[i]){if(P[i]!=w[i])return false;i++;}P+=i;return true;}
static float p_atom(void){
  if(*P=='('){P++;float v=p_expr();if(*P==')')P++;else perr=1;return v;}
  if((*P>='0'&&*P<='9')||*P=='.'){ float v=0;while(*P>='0'&&*P<='9'){v=v*10+(*P-'0');P++;}
    if(*P=='.'){P++;float f=0.1f;while(*P>='0'&&*P<='9'){v+=(*P-'0')*f;f/=10;P++;}} return v;}
  if(kw("pi"))return PI_;
  if(*P=='x'){P++;return VX;}
  int fn=0; if(kw("sqrt"))fn=1; else if(kw("sin"))fn=2; else if(kw("cos"))fn=3; else if(kw("tan"))fn=4;
  if(fn){ if(*P!='('){perr=1;return 0;} P++; float a=p_expr(); if(*P==')')P++; else perr=1;
    if(fn==1)return sqrt_(a); if(fn==2)return sin_(a); if(fn==3)return sin_(a+PI_/2);
    float c=sin_(a+PI_/2); if(c>-1e-12f&&c<1e-12f){perr=1;return 0;} return sin_(a)/c; }
  perr=1; return 0;}
static float p_unary(void);
static float p_pow(void){ float b=p_atom(); if(*P=='^'){P++; float e=p_unary(); return pw(b,e);} return b;}
static float p_unary(void){ if(*P=='-'){P++;return -p_unary();} if(*P=='+'){P++;return p_unary();} return p_pow();}
static float p_term(void){ float v=p_unary();
  for(;;){ if(*P=='*'){P++;v*=p_unary();} else if(*P=='/'){P++;float d=p_unary();if(d==0){perr=1;return 0;}v/=d;}
    else if(*P=='('||*P=='x'||*P=='p'||*P=='s'||*P=='c'||*P=='t'){v*=p_unary();} else break; }
  return v;}
static float p_expr(void){ float v=p_term(); for(;;){ if(*P=='+'){P++;v+=p_term();} else if(*P=='-'){P++;v-=p_term();} else break; } return v;}
static bool eval(const char*s,float x,float*out){ P=s;VX=x;perr=0; if(!*s)return false; float v=p_expr(); if(perr||*P)return false; *out=v; return true;}

// ---------- saisie de texte / nombre ----------
static const struct{u8 k;const char*s;} KM[]={{48,"0"},{42,"1"},{43,"2"},{44,"3"},{36,"4"},{37,"5"},{38,"6"},{30,"7"},{31,"8"},{32,"9"},
  {49,"."},{46,"-"},{45,"+"},{39,"*"},{40,"/"},{33,"("},{34,")"},{23,"^"},{29,"^2"},{14,"x"},{27,"pi"},{28,"sqrt("},{24,"sin("},{25,"cos("},{26,"tan("}};
#define NKM ((int)(sizeof(KM)/sizeof(KM[0])))
// retourne true si validé (OK/EXE), false si Retour
static bool edit_expr(const char*title,const char*prompt,char*buf,int cap,bool live){
  bool dirty=true; int n=slen(buf);
  for(;;){
    if(dirty){ bar(title);body();txt(12,36,prompt,C_MUTED,C_BG,false);
      fill(8,56,304,30,C_SEL); fill(10,58,300,26,C_BG); txt(14,64,buf,C_FG,C_BG,false);
      float v; char r[24]; if(live&&n>0){ if(eval(buf,0,&v)){fmtd(v,r);txt(14,100,"=",C_MUTED,C_BG,true);txt(34,100,r,C_FG,C_BG,true);} else txt(14,100,"...",C_MUTED,C_BG,false);}
      hint("OK/EXE: valider  <-: effacer  Retour"); dirty=false;}
    u64 e=poll();
    if(e&KB(K_BACK))return false;
    if(e&(KB(K_OK)|KB(K_EXE)))return true;
    if(e&KB(K_BKSP)){ if(n>0){buf[--n]=0;dirty=true;} }
    for(int i=0;i<NKM;i++) if(e&KB(KM[i].k)){ int l=slen(KM[i].s); if(n+l<cap){scpy(buf+n,KM[i].s);n+=l;dirty=true;} }
    wait(16);
  }}
static bool ask_number(const char*title,const char*prompt,float*out){
  char b[28]; b[0]=0; for(;;){ if(!edit_expr(title,prompt,b,26,true))return false; if(eval(b,0,out))return true; b[0]=0; } }

// =====================================================================
//                              JEUX
// =====================================================================
static int game_over(const char*name,int score){
  char b[24],s[40]; scpy(s,"Score : "); sapp(s,itoa_(score,b));
  fill(40,80,240,70,C_SEL); fill(42,82,236,66,C_BG);
  txtc(160,90,name,C_FG,C_BG,true); txtc(160,114,s,C_FG,C_BG,false); txtc(160,132,"OK: rejouer  Retour: quitter",C_MUTED,C_BG,false);
  for(;;){u64 e=poll();if(e&(KB(K_OK)|KB(K_EXE)))return 1;if(e&KB(K_BACK))return 0;wait(16);}}
static void score_txt(const char*l,int v,int x,int y){ char b[16],s[32];scpy(s,l);sapp(s,itoa_(v,b));sapp(s,"   ");bool tb=y<24; u16 bg=tb?BLK:C_BG,fg=tb?WHT:C_FG; fill(x,y,100,16,bg);txt(x,y,s,fg,bg,false);}

// ---- Snake ----
#define SNW 24
#define SNH 15
#define SNC 12
static void snake(void){
  do{
    static u8 sx[SNW*SNH],sy[SNW*SNH]; int len=4,dir=3,nd=3,score=0,fx,fy; fx=fy=0; 
    bar("Snake");body(); fill(16-2,32-2,SNW*SNC+4,SNH*SNC+4,C_FG); fill(16,32,SNW*SNC,SNH*SNC,C_BG); hint("Fleches: diriger  Retour: quitter");
    for(int i=0;i<len;i++){sx[i]=10-i;sy[i]=8;fill(16+sx[i]*SNC,32+sy[i]*SNC,SNC-1,SNC-1,i?RGB(60,160,60):RGB(30,110,30));}
    #define PLACE() do{ bool ok; do{ fx=rnd()%SNW; fy=rnd()%SNH; ok=true; for(int i=0;i<len;i++) if(sx[i]==fx&&sy[i]==fy) ok=false; }while(!ok); fill(16+fx*SNC,32+fy*SNC,SNC-1,SNC-1,RGB(220,50,50)); }while(0)
    PLACE(); score_txt("Score: ",score,190,5);
    u32 next=now(); bool dead=false;
    while(!dead){
      u64 e=poll(); if(e&KB(K_BACK))return;
      int d=-1; if(e&KB(K_LEFT))d=0; if(e&KB(K_UP))d=1; if(e&KB(K_DOWN))d=2; if(e&KB(K_RIGHT))d=3;
      if(d>=0&&d!=3-dir)nd=d;
      if((i32)(now()-next)>=0){ int spd=130-score*3; if(spd<60)spd=60; next=now()+spd; dir=nd;
        static const int DX[4]={-1,0,0,1},DY[4]={0,-1,1,0};
        int nx=sx[0]+DX[dir],ny=sy[0]+DY[dir];
        if(nx<0||ny<0||nx>=SNW||ny>=SNH)dead=true;
        else{ bool eat=(nx==fx&&ny==fy); int chk=eat?len:len-1;
          for(int i=0;i<chk;i++) if(sx[i]==nx&&sy[i]==ny)dead=true;
          if(!dead){ if(!eat){ fill(16+sx[len-1]*SNC,32+sy[len-1]*SNC,SNC-1,SNC-1,C_BG);} else len++;
            for(int i=len-1;i>0;i--){sx[i]=sx[i-1];sy[i]=sy[i-1];}
            fill(16+sx[0]*SNC,32+sy[0]*SNC,SNC-1,SNC-1,RGB(60,160,60));
            sx[0]=nx;sy[0]=ny; fill(16+nx*SNC,32+ny*SNC,SNC-1,SNC-1,RGB(30,110,30));
            if(eat){score++;score_txt("Score: ",score,190,5);PLACE();} } } }
      wait(8);
    }
    if(!game_over("Perdu !",score))return;
  }while(1);
}

// ---- Pong ----
static void pong(void){
  do{
    const int X0=8,Y0=28,W=304,H=196,PH=40,PW=6,BS=6; int py=H/2-PH/2,ay=H/2-PH/2; int bx=(W/2)<<4,by=(H/2)<<4; int vx=(rnd()&1?1:-1)*40,vy=(int)(rnd()%40)-20; int sp=0,sa=0;
    bar("Pong");body();fill(X0-2,Y0-2,W+4,H+4,C_FG);fill(X0,Y0,W,H,C_BG);hint("Haut/Bas: raquette  Retour: quitter");
    u32 next=now(); int opy=py,oay=ay,obx=bx>>4,oby=by>>4; char b[16];
    for(;;){
      u64 e=poll(); if(e&KB(K_BACK))return; (void)e;
      if((i32)(now()-next)>=0){ next=now()+16;
        if(held(K_UP))py-=5; if(held(K_DOWN))py+=5; if(py<0)py=0; if(py>H-PH)py=H-PH;
        int tc=(by>>4)-PH/2; if(ay<tc-3)ay+=3; else if(ay>tc+3)ay-=3; if(ay<0)ay=0; if(ay>H-PH)ay=H-PH;
        bx+=vx; by+=vy; int x=bx>>4,y=by>>4;
        if(y<0){by=0;vy=-vy;} if(y>H-BS){by=(H-BS)<<4;vy=-vy;}
        if(x<=10+PW&&x>=10&&y+BS>=py&&y<=py+PH&&vx<0){vx=-vx+2;vy+=((y+BS/2)-(py+PH/2))*2;}
        if(x+BS>=W-10-PW&&x+BS<=W-10&&y+BS>=ay&&y<=ay+PH&&vx>0){vx=-vx-2;vy+=((y+BS/2)-(ay+PH/2))*2;}
        if(vy>60)vy=60; if(vy<-60)vy=-60;
        bool out=false; if(x<-10){sa++;out=true;} if(x>W+10){sp++;out=true;}
        fill(X0+10,Y0+opy,PW,PH,C_BG);fill(X0+W-10-PW,Y0+oay,PW,PH,C_BG);fill(X0+obx,Y0+oby,BS,BS,C_BG);
        for(int i=0;i<H-8;i+=16)fill(X0+W/2-1,Y0+i,2,8,C_MUTED);
        if(out){ bx=(W/2)<<4;by=(H/2)<<4;vx=(sa>sp?-1:1)*40;vy=(int)(rnd()%40)-20; fill(X0,Y0,W,H,C_BG); wait(400); }
        x=bx>>4;y=by>>4;
        fill(X0+10,Y0+py,PW,PH,C_FG);fill(X0+W-10-PW,Y0+ay,PW,PH,C_FG);fill(X0+x,Y0+y,BS,BS,YEL);
        opy=py;oay=ay;obx=x;oby=y;
        char s[24];scpy(s,itoa_(sp,b));sapp(s," - ");sapp(s,itoa_(sa,b));fill(120,6,80,16,BLK);txtc(160,5,s,WHT,BLK,true);
        if(sp>=7||sa>=7){ if(!game_over(sp>=7?"Gagne !":"Perdu !",sp))return; break; }
      }
      wait(4);
    }
    sp=0;sa=0;
  }while(1);
}

// ---- 2048 ----
static u8 g48[4][4];
static void spawn48(void){ int c=0,xs[16],ys[16]; for(int y=0;y<4;y++)for(int x=0;x<4;x++)if(!g48[y][x]){xs[c]=x;ys[c]=y;c++;} if(!c)return; int i=rnd()%c; g48[ys[i]][xs[i]]=(rnd()%10)?1:2;}
static u16 col48(int v){ static const u16 C[12]={RGB(205,193,180),RGB(238,228,218),RGB(237,224,200),RGB(242,177,121),RGB(245,149,99),RGB(246,124,95),RGB(246,94,59),RGB(237,207,114),RGB(237,204,97),RGB(237,200,80),RGB(237,197,63),RGB(237,194,46)}; return C[v>11?11:v];}
static void draw48(int score){ for(int y=0;y<4;y++)for(int x=0;x<4;x++){ int v=g48[y][x]; int px=68+x*46,py=30+y*46; fill(px,py,44,44,col48(v));
    if(v){char b[8];itoa_(1<<v,b); txtc(px+22,py+13,b,v<3?RGB(110,100,90):WHT,col48(v),true);} }
  score_txt("Score: ",score,190,5);}
static bool slide48(int d,int*score){ // d: 0 gauche 1 haut 2 bas 3 droite
  bool moved=false; for(int l=0;l<4;l++){ u8 t[4]; int n=0; for(int i=0;i<4;i++){ int x=(d==0)?i:(d==3)?3-i:l, y=(d==1)?i:(d==2)?3-i:l; if(g48[y][x])t[n++]=g48[y][x]; }
    u8 o[4]={0,0,0,0}; int m=0; for(int i=0;i<n;i++){ if(i+1<n&&t[i]==t[i+1]){o[m++]=t[i]+1;*score+=1<<(t[i]+1);i++;} else o[m++]=t[i]; }
    for(int i=0;i<4;i++){ int x=(d==0)?i:(d==3)?3-i:l, y=(d==1)?i:(d==2)?3-i:l; if(g48[y][x]!=o[i])moved=true; g48[y][x]=o[i]; } }
  return moved;}
static bool can48(void){ for(int y=0;y<4;y++)for(int x=0;x<4;x++){ if(!g48[y][x])return true; if(x<3&&g48[y][x]==g48[y][x+1])return true; if(y<3&&g48[y][x]==g48[y+1][x])return true;} return false;}
static void game2048(void){
  do{ for(int y=0;y<4;y++)for(int x=0;x<4;x++)g48[y][x]=0; int score=0; spawn48();spawn48();
    bar("2048");body();hint("Fleches: glisser  Retour: quitter");draw48(score);
    for(;;){ u64 e=poll(); if(e&KB(K_BACK))return; int d=-1; if(e&KB(K_LEFT))d=0; if(e&KB(K_UP))d=1; if(e&KB(K_DOWN))d=2; if(e&KB(K_RIGHT))d=3;
      if(d>=0&&slide48(d,&score)){ spawn48(); draw48(score); if(!can48())break; } wait(16);}
    if(!game_over("Termine",score))return;
  }while(1);
}

// ---- Simon ----
static void simon_pad(int i,bool on){ static const u16 on_c[4]={RGB(60,220,60),RGB(240,60,60),RGB(70,110,250),RGB(250,220,50)}; static const u16 off_c[4]={RGB(20,90,20),RGB(100,25,25),RGB(30,45,110),RGB(110,95,20)};
  // 0 haut(vert) 1 droite(rouge) 2 bas(bleu) 3 gauche(jaune)
  static const int X[4]={110,190,110,30},Y[4]={30,90,150,90}; fill(X[i],Y[i],80,56,on?on_c[i]:off_c[i]);}
static void simon(void){
  do{ u8 seq[64]; int len=0; bar("Simon");body();hint("Retenez la suite  Retour: quitter"); for(int i=0;i<4;i++)simon_pad(i,false);
    bool dead=false; while(!dead&&len<60){ seq[len++]=rnd()&3; wait(500);
      for(int i=0;i<len;i++){simon_pad(seq[i],true);wait(420);simon_pad(seq[i],false);wait(180);}
      score_txt("Niveau: ",len,190,5);
      for(int i=0;i<len&&!dead;i++){ int k=-1; while(k<0){ u64 e=poll(); if(e&KB(K_BACK))return; if(e&KB(K_UP))k=0; if(e&KB(K_RIGHT))k=1; if(e&KB(K_DOWN))k=2; if(e&KB(K_LEFT))k=3; wait(10);} 
        simon_pad(k,true);wait(250);simon_pad(k,false); if(k!=seq[i])dead=true; } }
    if(!game_over("Perdu !",len-1))return;
  }while(1);
}

// ---- Tetris ----
static const char* const TS[7][4]={{"....","XXXX","....","...."},{".XX.",".XX.","....","...."},{".X..","XXX.","....","...."},{".XX.","XX..","....","...."},{"XX..",".XX.","....","...."},{"X...","XXX.","....","...."},{"..X.","XXX.","....","...."}};
static const u16 TC[7]={RGB(0,200,230),RGB(240,220,0),RGB(170,60,200),RGB(60,200,60),RGB(230,50,50),RGB(50,80,230),RGB(240,140,20)};
static u8 tb[20][10]; static u8 tp[4][4]; static int tpt,tx,ty;
static void tload(int t){ for(int y=0;y<4;y++)for(int x=0;x<4;x++)tp[y][x]=TS[t][y][x]=='X'; }
static void trot(u8 o[4][4]){ u8 n[4][4]; for(int y=0;y<4;y++)for(int x=0;x<4;x++)n[y][x]=o[3-x][y]; for(int y=0;y<4;y++)for(int x=0;x<4;x++)o[y][x]=n[y][x];}
static bool tfit(u8 p[4][4],int px,int py){ for(int y=0;y<4;y++)for(int x=0;x<4;x++) if(p[y][x]){ int bx=px+x,by=py+y; if(bx<0||bx>=10||by>=20)return false; if(by>=0&&tb[by][bx])return false;} return true;}
static void tcell(int x,int y,u16 c){ fill(110+x*10,28+y*10,9,9,c);}
static void tdraw(void){ for(int y=0;y<20;y++)for(int x=0;x<10;x++)tcell(x,y,tb[y][x]?TC[tb[y][x]-1]:RGB(235,235,240));
  for(int y=0;y<4;y++)for(int x=0;x<4;x++) if(tp[y][x]&&ty+y>=0)tcell(tx+x,ty+y,TC[tpt]);}
static void tetris(void){
  do{ for(int y=0;y<20;y++)for(int x=0;x<10;x++)tb[y][x]=0; int score=0,lines=0; int nxt=rnd()%7;
    bar("Tetris");body();fill(108,26,104,204,C_FG);hint("<->: bouger  Haut: tourner  OK: chute");
    bool dead=false; int level=0;
    while(!dead){ tpt=nxt; nxt=rnd()%7; tload(tpt); tx=3;ty=-1; if(!tfit(tp,tx,ty)){break;}
      fill(230,60,80,60,C_BG); for(int y=0;y<4;y++)for(int x=0;x<4;x++) if(TS[nxt][y][x]=='X')fill(240+x*10,70+y*10,9,9,TC[nxt]);
      txt(232,40,"Suivant",C_MUTED,C_BG,false);
      score_txt("Score:",score,222,140); score_txt("Lignes:",lines,222,160);
      u32 next=now(); bool landed=false;
      while(!landed){ u64 e=poll(); if(e&KB(K_BACK))return;
        if(e&KB(K_LEFT)&&tfit(tp,tx-1,ty))tx--; if(e&KB(K_RIGHT)&&tfit(tp,tx+1,ty))tx++;
        if(e&KB(K_UP)){ u8 r[4][4]; for(int y=0;y<4;y++)for(int x=0;x<4;x++)r[y][x]=tp[y][x]; trot(r); static const int KK[5]={0,-1,1,-2,2}; for(int i=0;i<5;i++) if(tfit(r,tx+KK[i],ty)){ for(int y=0;y<4;y++)for(int x=0;x<4;x++)tp[y][x]=r[y][x]; tx+=KK[i]; break; } }
        if(e&(KB(K_OK)|KB(K_EXE))){ while(tfit(tp,tx,ty+1))ty++; next=0; }
        int gi=500-level*40; if(gi<90)gi=90; if(held(K_DOWN))gi=40;
        if((i32)(now()-next)>=0){ if(tfit(tp,tx,ty+1)){ty++;next=now()+gi;} else landed=true; }
        tdraw(); wait(10); }
      for(int y=0;y<4;y++)for(int x=0;x<4;x++) if(tp[y][x]){ if(ty+y<0)dead=true; else tb[ty+y][tx+x]=tpt+1; }
      int cl=0; for(int y=19;y>=0;y--){ bool full=true; for(int x=0;x<10;x++)if(!tb[y][x])full=false; if(full){ cl++; for(int k=y;k>0;k--)for(int x=0;x<10;x++)tb[k][x]=tb[k-1][x]; for(int x=0;x<10;x++)tb[0][x]=0; y++; } }
      if(cl){ static const int PTS[5]={0,100,300,500,800}; score+=PTS[cl]; lines+=cl; level=lines/10; }
    }
    if(!game_over("Perdu !",score))return;
  }while(1);
}

static bool g_quit=false;
static void jeux(bool nwa){
  static const char*const it[]={"Jeux .nwa (liste)","Snake","Pong","2048","Simon","Tetris"}; int s=0; const char*const*L=nwa?it:it+1; int n=nwa?6:5; int o=nwa?0:1;
  for(;;){ s=menu(nwa?"Jeux":"Mini-jeux",L,n,"OK: lancer  Retour",s); if(s<0)return; s+=o;
    if(s==0){ g_quit=true; return; }
    else if(s==1)snake(); else if(s==2)pong(); else if(s==3)game2048(); else if(s==4)simon(); else tetris(); }}

// =====================================================================
//                        BLOC-NOTES / FICHIERS
// =====================================================================
#define NOTE_N 4
#define NOTE_L 120
static char notes[NOTE_N][NOTE_L+1];
static void notepad(int slot){
  static const char CH[]=" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,;:!?'-+*/=()"; int nch=sizeof(CH)-1; int ci=1; bool dirty=true; int n=slen(notes[slot]);
  for(;;){
    if(dirty){ char t[24];scpy(t,"Bloc-notes ");char b[4];sapp(t,itoa_(slot+1,b));bar(t);body();
      // texte sur lignes de 42 caractères
      for(int l=0;l<3;l++){ char ln[44];int k=0; for(int i=l*42;i<n&&i<(l+1)*42;i++)ln[k++]=notes[slot][i]; ln[k]=0; txt(10,34+l*18,ln,C_FG,C_BG,false);}
      char c[2]={CH[ci],0}; fill(10,130,300,50,C_MUTED); fill(12,132,296,46,C_BG); txt(18,140,"Caractere :",C_MUTED,C_BG,false); fill(120,136,30,34,C_SEL); txtc(135,144,c[0]==' '?"_":c,C_SELTXT,C_SEL,true);
      txt(170,140,"[haut/bas]",C_MUTED,C_BG,false); hint("OK: ajouter  <-: effacer  </>: note  Retour"); dirty=false; }
    u64 e=poll();
    if(e&KB(K_BACK))return;
    if(e&KB(K_UP)){ci=(ci+1)%nch;dirty=true;} if(e&KB(K_DOWN)){ci=(ci+nch-1)%nch;dirty=true;}
    if(e&(KB(K_OK)|KB(K_EXE))){ if(n<NOTE_L){notes[slot][n++]=CH[ci];notes[slot][n]=0;dirty=true;} }
    if(e&KB(K_BKSP)){ if(n>0){notes[slot][--n]=0;dirty=true;} }
    if(e&KB(K_LEFT)){slot=(slot+NOTE_N-1)%NOTE_N;n=slen(notes[slot]);dirty=true;} if(e&KB(K_RIGHT)){slot=(slot+1)%NOTE_N;n=slen(notes[slot]);dirty=true;}
    for(int i=0;i<10;i++) if(e&KB(KM[i].k)){ if(n<NOTE_L){notes[slot][n++]=KM[i].s[0];notes[slot][n]=0;dirty=true;} }
    wait(16);
  }}
static void files(void){
  static const char*const it[]={"Jeux","Programmes","Documents","Favoris","Telechargements"}; int s=0;
  for(;;){ s=menu("Fichiers",it,5,"OK: ouvrir  Retour (stockage temporaire)",s); if(s<0)return;
    if(s==2){ const char*d[NOTE_N]; static char lab[NOTE_N][24]; for(int i=0;i<NOTE_N;i++){ char b[4]; scpy(lab[i],"Note ");sapp(lab[i],itoa_(i+1,b)); sapp(lab[i],notes[i][0]?" - texte":" (vide)"); d[i]=lab[i]; }
      int k=menu("Documents",d,NOTE_N,"OK: ouvrir  Retour",0); if(k>=0)notepad(k); }
    else message(it[s],"Dossier vide.","Les apps NumWorks n'ont pas acces","aux fichiers de la calculatrice.");
  }}

// =====================================================================
//                    HORLOGE / MINUTEUR
// =====================================================================
static u32 ck_last,ck_secs=12*3600,ck_d=15,ck_m=11,ck_y=2026; static bool ck_init;
static int dim(int m,int y){ static const u8 D[12]={31,28,31,30,31,30,31,31,30,31,30,31}; if(m==2&&((y%4==0&&y%100)||y%400==0))return 29; return D[m-1];}
static void ck_tick(void){ u32 t=now(); if(!ck_init){ck_last=t;ck_init=true;} u32 s=(t-ck_last)/1000; if(s){ck_last+=s*1000;ck_secs+=s; while(ck_secs>=86400){ck_secs-=86400; if(++ck_d>(u32)dim(ck_m,ck_y)){ck_d=1;if(++ck_m>12){ck_m=1;ck_y++;}}}}}
static void bigdigit(int x,int y,int d,u16 fg,u16 bg){
  static const u8 SEG[10]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
  const int w=30,h=56,t=6,L=(h-3*t)/2; u8 s=SEG[d]; fill(x,y,w,h,bg);
  if(s&1)fill(x+t,y,w-2*t,t,fg); if(s&2)fill(x+w-t,y+t,t,L,fg); if(s&4)fill(x+w-t,y+2*t+L,t,L,fg);
  if(s&8)fill(x+t,y+h-t,w-2*t,t,fg); if(s&16)fill(x,y+2*t+L,t,L,fg); if(s&32)fill(x,y+t,t,L,fg); if(s&64)fill(x+t,y+t+L,w-2*t,t,fg);}
static void bigtime(int y,u32 a,u32 b,u32 c,int hl){
  u32 v[3]={a,b,c}; int x0=50;
  for(int i=0;i<3;i++){ int x=x0+i*78; u16 fg=(hl==i)?YEL:C_FG;
    bigdigit(x,y,(v[i]/10)%10,fg,C_BG); bigdigit(x+34,y,v[i]%10,fg,C_BG); fill(x,y+62,64,4,hl==i?YEL:C_BG);
    if(i<2){ fill(x+68,y+16,6,6,C_FG); fill(x+68,y+34,6,6,C_FG);} } }
static void clock_(void){
  static const char*const DN[7]={"Dim.","Lun.","Mar.","Mer.","Jeu.","Ven.","Sam."};
  static const char*const MN[12]={"Janv.","Fevr.","Mars","Avr.","Mai","Juin","Juil.","Aout","Sept.","Oct.","Nov.","Dec."};
  int edit=-1; u32 last=0xFFFFFFFF; bool dirty=true;
  for(;;){ ck_tick(); u64 e=poll();
    if(e&KB(K_BACK)){ if(edit>=0){edit=-1;dirty=true;} else return; }
    if(edit<0&&(e&(KB(K_OK)|KB(K_EXE)))){edit=0;dirty=true;}
    else if(edit>=0){ if(e&(KB(K_OK)|KB(K_EXE))){edit=-1;dirty=true;}
      if(e&KB(K_LEFT)){edit=(edit+5)%6;dirty=true;} if(e&KB(K_RIGHT)){edit=(edit+1)%6;dirty=true;}
      int dl=0; if(e&KB(K_UP))dl=1; if(e&KB(K_DOWN))dl=-1;
      if(dl){ dirty=true; u32 h=ck_secs/3600,mi=(ck_secs/60)%60,sc=ck_secs%60;
        if(edit==0)h=(h+24+dl)%24;
        if(edit==1)mi=(mi+60+dl)%60;
        if(edit==2)sc=(sc+60+dl)%60;
        ck_secs=h*3600+mi*60+sc;
        if(edit==3){ck_d=(ck_d-1+dim(ck_m,ck_y)+dl)%dim(ck_m,ck_y)+1;}
        if(edit==4){ck_m=(ck_m-1+12+dl)%12+1; if(ck_d>(u32)dim(ck_m,ck_y))ck_d=dim(ck_m,ck_y);}
        if(edit==5){ck_y=ck_y+dl; if(ck_y<2000)ck_y=2000; if(ck_d>(u32)dim(ck_m,ck_y))ck_d=dim(ck_m,ck_y);} } }
    if(dirty||last!=ck_secs){ last=ck_secs; dirty=false; bar("Horloge"); body();
      bigtime(50,ck_secs/3600,(ck_secs/60)%60,ck_secs%60,edit<3?edit:-1);
      static const int T[12]={0,3,2,5,0,3,5,1,4,6,2,4}; int y=ck_y; if(ck_m<3)y--; int dw=(y+y/4-y/100+y/400+T[ck_m-1]+(int)ck_d)%7;
      char b[8],l[40]; scpy(l,DN[dw]);sapp(l," ");sapp(l,itoa_(ck_d,b));sapp(l," ");sapp(l,MN[ck_m-1]);sapp(l," ");sapp(l,itoa_(ck_y,b));
      txtc(160,150,l,edit>=3?YEL:C_FG,C_BG,true);
      if(edit>=3)txtc(160,176,edit==3?"jour":edit==4?"mois":"annee",C_MUTED,C_BG,false);
      hint(edit<0?"OK: regler l'heure et la date  Retour":"</>: champ  haut/bas: valeur  OK: fini"); }
    wait(30);
  }}

static void timer_(void){
  i32 total=300,left=300; bool run=false; u32 endt=0; int sel=0; bool dirty=true; i32 shown=-1; bool done=false;
  for(;;){ u64 e=poll();
    if(e&KB(K_BACK))return;
    if(!run&&!done){ if(e&KB(K_LEFT)){sel=0;dirty=true;} if(e&KB(K_RIGHT)){sel=1;dirty=true;}
      int dl=0; if(e&KB(K_UP))dl=1; if(e&KB(K_DOWN))dl=-1; if(dl){ total+=dl*(sel?5:60); if(total<0)total=0; if(total>359999)total=359999; left=total; dirty=true; } }
    if(e&(KB(K_OK)|KB(K_EXE))){ if(done){done=false;left=total;dirty=true;} else if(run){ run=false; left=(i32)(((i32)(endt-now())+999)/1000); dirty=true; } else if(left>0){ run=true; endt=now()+(u32)left*1000; dirty=true; } }
    if(e&KB(K_BKSP)){ run=false;done=false;left=total;dirty=true; }
    if(run){ i32 l=(i32)(((i32)(endt-now())+999)/1000); if(l<0)l=0; if(l!=left){left=l;dirty=true;} if(l==0){run=false;done=true;dirty=true;} }
    if(dirty||shown!=left){ shown=left; dirty=false; bar("Minuteur"); body();
      bigtime(40,left/3600,(left/60)%60,left%60,(!run&&!done)?(sel?2:1):-1);
      fill(60,124,200,34,C_SEL); txtc(160,133,done?"Termine ! (OK)":run?"Pause":"Demarrer",C_SELTXT,C_SEL,false);
      fill(60,168,200,34,C_MUTED); fill(62,170,196,30,C_BG); txtc(160,177,"Reinitialiser (<-)",C_FG,C_BG,false);
      hint(run?"OK: pause  <-: reinitialiser":(done?"OK: recommencer":"</>: min/sec  haut/bas: regler  OK")); }
    wait(30);
  }}

// =====================================================================
//                         OUTILS AVANCES
// =====================================================================
static void calc(void){ char b[40]; b[0]=0; for(;;){ if(!edit_expr("Calculatrice","Expression :",b,36,true))return; } }
static void solver(void){
  float a,b,c; if(!ask_number("Resolveur","a (ax^2+bx+c=0) :",&a))return; if(!ask_number("Resolveur","b :",&b))return; if(!ask_number("Resolveur","c :",&c))return;
  char l1[40],l2[40],t[24]; l2[0]=0;
  if(a==0){ if(b==0){scpy(l1,c==0?"Infinite de solutions":"Aucune solution");} else {scpy(l1,"x = ");fmtd(-c/b,t);sapp(l1,t);} }
  else { float d=b*b-4*a*c; if(d>=0){ float s=sqrt_(d); scpy(l1,"x1 = ");fmtd((-b-s)/(2*a),t);sapp(l1,t); scpy(l2,"x2 = ");fmtd((-b+s)/(2*a),t);sapp(l2,t);} 
    else { float s=sqrt_(-d)/(2*a); float re=-b/(2*a); scpy(l1,"x = ");fmtd(re,t);sapp(l1,t);sapp(l1," +/- ");fmtd(s<0?-s:s,t);sapp(l1,t);sapp(l1,"i"); scpy(l2,"(solutions complexes)"); } }
  message("Resolveur",l1,l2[0]?l2:0,0);}
static void deriv(void){
  char f[36]; f[0]=0; if(!edit_expr("Derivees/Integrales","f(x) = (variable x):",f,34,false))return; float v;
  if(!eval(f,1,&v)){message("Erreur","Expression invalide.",0,0);return;}
  static const char*const it[]={"Derivee en x0","Integrale entre a et b"}; int s=menu("f(x)",it,2,"OK: choisir  Retour",0); if(s<0)return;
  char t[24],l[48];
  if(s==0){ float x0; if(!ask_number("Derivee","x0 :",&x0))return; float h=1e-4f,y1,y2; eval(f,x0+h,&y1);eval(f,x0-h,&y2); scpy(l,"f'(x0) = ");fmtd((y1-y2)/(2*h),t);sapp(l,t); message("Derivee",l,0,0);}
  else{ float a,b; if(!ask_number("Integrale","a :",&a))return; if(!ask_number("Integrale","b :",&b))return; int n=400; float h=(b-a)/n,sum=0,y; 
    for(int i=0;i<=n;i++){ eval(f,a+i*h,&y); sum+=y*((i==0||i==n)?1:(i&1)?4:2);} scpy(l,"Integrale = ");fmtd(sum*h/3,t);sapp(l,t); message("Integrale",l,0,0);} }
static void stats(void){
  float v[30]; int n=0; bool dirty=true;
  for(;;){ if(dirty){ bar("Statistiques");body(); char b[28],t[24];
      txt(10,30,"Valeurs :",C_MUTED,C_BG,false); { char l[44];l[0]=0; for(int i=0;i<n&&i<6;i++){fmtd(v[i],t);sapp(l,t);sapp(l," ");} if(n>6)sapp(l,"..."); txt(80,30,l,C_FG,C_BG,false);} 
      if(n>0){ float s[30],sum=0; for(int i=0;i<n;i++){s[i]=v[i];sum+=v[i];} for(int i=1;i<n;i++){float k=s[i];int j=i-1;while(j>=0&&s[j]>k){s[j+1]=s[j];j--;}s[j+1]=k;}
        float mean=sum/n,ss=0; for(int i=0;i<n;i++)ss+=(v[i]-mean)*(v[i]-mean); float med=(n&1)?s[n/2]:(s[n/2-1]+s[n/2])/2; 
        char l[44]; int y=60; 
        scpy(l,"n = ");sapp(l,itoa_(n,b));txt(10,y,l,C_FG,C_BG,false);y+=22;
        scpy(l,"Moyenne = ");fmtd(mean,t);sapp(l,t);txt(10,y,l,C_FG,C_BG,false);y+=22;
        scpy(l,"Mediane = ");fmtd(med,t);sapp(l,t);txt(10,y,l,C_FG,C_BG,false);y+=22;
        scpy(l,"Ecart-type = ");fmtd(sqrt_(ss/n),t);sapp(l,t);txt(10,y,l,C_FG,C_BG,false);y+=22;
        scpy(l,"Min = ");fmtd(s[0],t);sapp(l,t);sapp(l,"  Max = ");fmtd(s[n-1],t);sapp(l,t);txt(10,y,l,C_FG,C_BG,false);} 
      else txt(10,70,"Aucune valeur. Appuyez sur OK.",C_FG,C_BG,false);
      hint("OK: ajouter  <-: retirer  Retour"); dirty=false; }
    u64 e=poll(); if(e&KB(K_BACK))return;
    if(e&(KB(K_OK)|KB(K_EXE))){ float x; if(n<30&&ask_number("Statistiques","Nouvelle valeur :",&x))v[n++]=x; dirty=true; }
    if(e&KB(K_BKSP)){ if(n>0){n--;dirty=true;} }
    wait(16);
  }}

// ---- table périodique ----
static const char*const SYM[118]={"H","He","Li","Be","B","C","N","O","F","Ne","Na","Mg","Al","Si","P","S","Cl","Ar","K","Ca","Sc","Ti","V","Cr","Mn","Fe","Co","Ni","Cu","Zn","Ga","Ge","As","Se","Br","Kr","Rb","Sr","Y","Zr","Nb","Mo","Tc","Ru","Rh","Pd","Ag","Cd","In","Sn","Sb","Te","I","Xe","Cs","Ba","La","Ce","Pr","Nd","Pm","Sm","Eu","Gd","Tb","Dy","Ho","Er","Tm","Yb","Lu","Hf","Ta","W","Re","Os","Ir","Pt","Au","Hg","Tl","Pb","Bi","Po","At","Rn","Fr","Ra","Ac","Th","Pa","U","Np","Pu","Am","Cm","Bk","Cf","Es","Fm","Md","No","Lr","Rf","Db","Sg","Bh","Hs","Mt","Ds","Rg","Cn","Nh","Fl","Mc","Lv","Ts","Og"};
static const char*const NOM[118]={"Hydrogène","Hélium","Lithium","Béryllium","Bore","Carbone","Azote","Oxygène","Fluor","Néon","Sodium","Magnésium","Aluminium","Silicium","Phosphore","Soufre","Chlore","Argon","Potassium","Calcium","Scandium","Titane","Vanadium","Chrome","Manganèse","Fer","Cobalt","Nickel","Cuivre","Zinc","Gallium","Germanium","Arsenic","Sélénium","Brome","Krypton","Rubidium","Strontium","Yttrium","Zirconium","Niobium","Molybdène","Technétium","Ruthénium","Rhodium","Palladium","Argent","Cadmium","Indium","Étain","Antimoine","Tellure","Iode","Xénon","Césium","Baryum","Lanthane","Cérium","Praséodyme","Néodyme","Prométhium","Samarium","Europium","Gadolinium","Terbium","Dysprosium","Holmium","Erbium","Thulium","Ytterbium","Lutécium","Hafnium","Tantale","Tungstène","Rhénium","Osmium","Iridium","Platine","Or","Mercure","Thallium","Plomb","Bismuth","Polonium","Astate","Radon","Francium","Radium","Actinium","Thorium","Protactinium","Uranium","Neptunium","Plutonium","Américium","Curium","Berkélium","Californium","Einsteinium","Fermium","Mendélévium","Nobélium","Lawrencium","Rutherfordium","Dubnium","Seaborgium","Bohrium","Hassium","Meitnérium","Darmstadtium","Roentgenium","Copernicium","Nihonium","Flérovium","Moscovium","Livermorium","Tennesse","Oganesson"};
static void zpos(int z,int*r,int*c){
  if(z==1){*r=0;*c=0;} else if(z==2){*r=0;*c=17;}
  else if(z<=4){*r=1;*c=z-3;} else if(z<=10){*r=1;*c=12+z-5;}
  else if(z<=12){*r=2;*c=z-11;} else if(z<=18){*r=2;*c=12+z-13;}
  else if(z<=36){*r=3;*c=z-19;} else if(z<=54){*r=4;*c=z-37;}
  else if(z<=56){*r=5;*c=z-55;} else if(z<=71){*r=8;*c=z-57+2;} else if(z<=86){*r=5;*c=3+z-72;}
  else if(z<=88){*r=6;*c=z-87;} else if(z<=103){*r=9;*c=z-89+2;} else {*r=6;*c=3+z-104;}}
static void periodic(void){
  static u8 grid[10][18]; for(int r=0;r<10;r++)for(int c=0;c<18;c++)grid[r][c]=0;
  for(int z=1;z<=118;z++){int r,c;zpos(z,&r,&c);grid[r][c]=z;}
  int cr=0,cc=0; bool dirty=true; bar("Table periodique");
  for(;;){ if(dirty){ body(); int sel=grid[cr][cc];
      for(int r=0;r<10;r++)for(int c=0;c<18;c++){ int z=grid[r][c]; if(!z)continue; int x=5+c*17,y=28+r*17; bool s=(z==sel);
        u16 bg=s?YEL:(z>=57&&z<=71)||(z>=89&&z<=103)?RGB(200,230,200):(z<=2||(z>=5&&z<=10&&z!=7&&z!=8)||z==1)?RGB(230,235,250):RGB(235,235,235);
        if(s)bg=YEL; fill(x,y,16,16,bg); const char*sy=SYM[z-1]; int w=ulen(sy)*7; txt(x+(16-w)/2,y+1,sy,BLK,bg,false);} 
      char b[8],l[40]; scpy(l,itoa_(sel,b)); sapp(l,"  ");sapp(l,SYM[sel-1]);sapp(l,"  ");sapp(l,NOM[sel-1]); fill(0,200,320,24,C_BG); txt(10,202,l,C_FG,C_BG,true); hint("Fleches: naviguer  Retour"); dirty=false; }
    u64 e=poll(); if(e&KB(K_BACK))return;
    int dr=0,dc=0; if(e&KB(K_LEFT))dc=-1; if(e&KB(K_RIGHT))dc=1; if(e&KB(K_UP))dr=-1; if(e&KB(K_DOWN))dr=1;
    if(dr||dc){ int r=cr,c=cc; for(int k=0;k<18;k++){ r+=dr; c+=dc; if(r<0||r>9||c<0||c>17)break; if(grid[r][c]){cr=r;cc=c;dirty=true;break;} } }
    wait(16);
  }}

// ---- convertisseur ----
typedef struct{const char*n;float f;}Unit;
static const Unit ULEN[]={{"m",1},{"km",1000},{"cm",0.01f},{"mm",0.001f},{"mile",1609.344f},{"pouce",0.0254f},{"pied",0.3048f}};
static const Unit UMAS[]={{"kg",1},{"g",0.001f},{"mg",1e-6f},{"livre",0.45359237f},{"once",0.028349523f}};
static const Unit USPD[]={{"m/s",1},{"km/h",1/3.6f},{"mph",0.44704f},{"noeud",0.514444f}};
static const Unit UTMP[]={{"C",0},{"F",0},{"K",0}};
static float temp_to_c(float v,int u){ return u==0?v:u==1?(v-32)*5/9:v-273.15f;}
static float temp_from_c(float v,int u){ return u==0?v:u==1?v*9/5+32:v+273.15f;}
static void convert(void){
  static const char*const cat[]={"Longueur","Masse","Vitesse","Temperature"}; int c=0;
  for(;;){ c=menu("Convertisseur",cat,4,"OK: choisir  Retour",c); if(c<0)return;
    const Unit*U=c==0?ULEN:c==1?UMAS:c==2?USPD:UTMP; int nu=c==0?7:c==1?5:c==2?4:3; int a=0,b=1,row=0; float val=1; bool dirty=true;
    for(;;){ if(dirty){ bar(cat[c]);body(); char t[24],l[48]; 
        float r; if(c==3)r=temp_from_c(temp_to_c(val,a),b); else r=val*U[a].f/U[b].f;
        for(int i=0;i<3;i++){ int y=40+i*40; bool s=(row==i); fill(10,y,300,32,s?C_SEL:C_BG); u16 fg=s?C_SELTXT:C_FG,bg=s?C_SEL:C_BG;
          if(i==0){fmtd(val,t);scpy(l,"Valeur : ");sapp(l,t);} else if(i==1){scpy(l,"De : < ");sapp(l,U[a].n);sapp(l," >");} else {scpy(l,"Vers : < ");sapp(l,U[b].n);sapp(l," >");} txt(18,y+8,l,fg,bg,false);}
        fmtd(r,t);scpy(l,"Resultat : ");sapp(l,t);sapp(l," ");sapp(l,U[b].n); txt(14,170,l,C_FG,C_BG,true); hint("haut/bas: ligne  </>: unite  OK: valeur"); dirty=false; }
      u64 e=poll(); if(e&KB(K_BACK))break;
      if(e&KB(K_UP)){row=(row+2)%3;dirty=true;} if(e&KB(K_DOWN)){row=(row+1)%3;dirty=true;}
      if(row==1){ if(e&KB(K_LEFT)){a=(a+nu-1)%nu;dirty=true;} if(e&KB(K_RIGHT)){a=(a+1)%nu;dirty=true;} }
      if(row==2){ if(e&KB(K_LEFT)){b=(b+nu-1)%nu;dirty=true;} if(e&KB(K_RIGHT)){b=(b+1)%nu;dirty=true;} }
      if(row==0&&(e&(KB(K_OK)|KB(K_EXE)))){ float x; if(ask_number("Valeur","Valeur a convertir :",&x))val=x; dirty=true; }
      wait(16); } }}

static void outils(void){
  static const char*const it[]={"Calculatrice avancee","Resolveur d'equations","Derivees / Integrales","Statistiques","Table periodique","Convertisseur avance"}; int s=0;
  for(;;){ s=menu("Outils",it,6,"OK: ouvrir  Retour",s); if(s<0)return;
    if(s==0)calc(); else if(s==1)solver(); else if(s==2)deriv(); else if(s==3)stats(); else if(s==4)periodic(); else convert(); }}

// =====================================================================
//                       PARAMETRES / A PROPOS
// =====================================================================
static void about(void){
  bar("A propos");body(); fill(130,34,60,60,YEL); for(int i=0;i<32;i++)fill(144+i*26/32,46+i*30/32,8,8,BLK); fill(142,44,8,40,BLK); fill(170,44,8,40,BLK);
  txtc(160,110,"NumWorks Userland",C_FG,C_BG,true); txtc(160,134,"v1.0",C_FG,C_BG,false); txtc(160,158,"Merci d'utiliser NumWorks !",C_FG,C_BG,false);
  fill(154,180,6,6,RGB(220,40,70));fill(162,180,6,6,RGB(220,40,70));fill(152,184,18,6,RGB(220,40,70));fill(156,190,10,4,RGB(220,40,70));
  hint("Retour"); for(;;){u64 e=poll();if(e&(KB(K_BACK)|KB(K_OK)|KB(K_LEFT)))return;wait(16);} }
static void settings(void){
  int sel=0; bool dirty=true;
  for(;;){ if(dirty){ bar("Parametres");body(); static const char*const L[5]={"Theme","Luminosite","Son","Langue","A propos"};
      for(int i=0;i<5;i++){ int y=34+i*34; bool s=(sel==i); u16 bg=s?C_SEL:C_BG,fg=s?C_SELTXT:C_FG; fill(8,y,304,30,bg); txt(18,y+8,L[i],fg,bg,false);
        if(i==0)txt(210,y+8,g_dark?"Sombre >":"Clair >",fg,bg,false);
        if(i==1){ fill(180,y+11,110,8,C_MUTED); fill(180,y+11,g_bright*110/240,8,s?BLK:YEL); }
        if(i==2)txt(190,y+8,"(non gere)",fg,bg,false);
        if(i==3)txt(210,y+8,"Francais",fg,bg,false); }
      hint("haut/bas: choisir  </>: modifier  Retour"); dirty=false; }
    u64 e=poll(); if(e&KB(K_BACK))return;
    if(e&KB(K_UP)){sel=(sel+4)%5;dirty=true;} if(e&KB(K_DOWN)){sel=(sel+1)%5;dirty=true;}
    if(sel==0&&(e&(KB(K_LEFT)|KB(K_RIGHT)|KB(K_OK)))){g_dark=!g_dark;theme();dirty=true;}
    if(sel==1){ if(e&KB(K_LEFT)){g_bright-=20;if(g_bright<10)g_bright=10;eadk_backlight_set_brightness((u8)g_bright);dirty=true;} if(e&KB(K_RIGHT)){g_bright+=20;if(g_bright>240)g_bright=240;eadk_backlight_set_brightness((u8)g_bright);dirty=true;} }
    if(sel==4&&(e&(KB(K_OK)|KB(K_EXE)|KB(K_RIGHT)))){about();dirty=true;}
    wait(16);
  }}

// =====================================================================
//                              MENU PRINCIPAL
// =====================================================================
int main(int argc,char*argv[]){
  (void)argc;(void)argv; theme(); g_prev=eadk_keyboard_scan();
  static const char*const it[]={"Jeux","Mini-jeux","Bloc-notes","Horloge","Minuteur","Outils avances","Fichiers","Parametres","A propos"}; int s=0;
  for(;;){ s=menu("Accueil",it,9,"OK: ouvrir  Retour: quitter",s); if(s<0)return 0;
    switch(s){ case 0: jeux(true); if(g_quit)return 0; break; case 1: jeux(false); break; case 2: notepad(0); break; case 3: clock_(); break; case 4: timer_(); break; case 5: outils(); break; case 6: files(); break; case 7: settings(); break; default: about(); }
  } }
