#define main app_main
#include "../src/main.c"
#undef main
#include <stdio.h>
#include <string.h>
#include <math.h>
static int fails=0;
#define CHECK(c,msg) do{ if(!(c)){printf("ECHEC: %s\n",msg);fails++;} }while(0)
int main(void){
  char b[40];
  // formatage
  struct{float v;const char*s;}F[]={{0,"0"},{1,"1"},{-2.5f,"-2.5"},{3.14159265f,"3.141593"},{0.1f,"0.1"},{100,"100"},{1234567,"1234567"},{0.00012f,"0.00012"},{1e-7f,"1E-7"},{1.5e12f,"1.5E12"},{0.1f+0.2f,"0.3"},{-0.000001f,"-1E-6"},{5e-5f,"5E-5"},{0.0001f,"0.0001"},{2.0f/3,"0.6666667"},{1e7f,"1E7"},{999999.9f,"1000000"},{12345678,"1.234568E7"}};
  for(unsigned i=0;i<sizeof(F)/sizeof(F[0]);i++){ fmtd(F[i].v,b); if(strcmp(b,F[i].s)){printf("fmtd(%g)=%s attendu %s\n",(double)F[i].v,b,F[i].s);fails++;} }
  // évaluateur
  struct{const char*e;float x;float r;int ok;}E[]={{"1+2*3",0,7,1},{"(1+2)*3",0,9,1},{"2^10",0,1024,1},{"-3^2",0,-9,1},{"2x+1",3,7,1},{"sqrt(16)",0,4,1},{"1/4",0,0.25,1},{"10/3",0,3.3333333,1},{"2pi",0,6.2831853,1},{"sin(pi/2)",0,1,1},{"cos(0)",0,1,1},{"tan(pi/4)",0,1,1},{"1/0",0,0,0},{"2+",0,0,0},{"sqrt(-1)",0,0,0},{"3(4+1)",0,15,1},{"2^-1",0,0.5,1},{"x^2",5,25,1},{"2^0.5",0,0,0}};
  for(unsigned i=0;i<sizeof(E)/sizeof(E[0]);i++){ float v=0; bool ok=eval(E[i].e,E[i].x,&v); if(ok!=(bool)E[i].ok||(ok&&fabs(v-E[i].r)>1e-3)){printf("eval(%s)=%g ok=%d attendu %g ok=%d\n",E[i].e,v,ok,E[i].r,E[i].ok);fails++;} }
  // sqrt/sin
  CHECK(fabs(sqrt_(2)-1.41421356f)<1e-5,"sqrt"); CHECK(fabs(sin_(1)-0.841470985f)<1e-5,"sin"); CHECK(fabs(sin_(100)-sin(100))<1e-3,"sin grand");
  // table périodique
  { static u8 g[10][18]; int cnt=0; for(int z=1;z<=118;z++){int r,c;zpos(z,&r,&c); CHECK(r>=0&&r<10&&c>=0&&c<18,"zpos hors grille"); CHECK(!g[r][c],"zpos collision"); g[r][c]=z; cnt++;} CHECK(cnt==118,"118"); 
    int ns=0,nn=0; for(int i=0;i<118;i++){ if(SYM[i]&&SYM[i][0])ns++; if(NOM[i]&&NOM[i][0])nn++; } CHECK(ns==118&&nn==118,"noms/symboles"); 
    CHECK(!strcmp(SYM[25],"Fe")&&!strcmp(SYM[78],"Au")&&!strcmp(SYM[117],"Og")&&!strcmp(SYM[91],"U"),"symboles cles");
    int r,c; zpos(26,&r,&c); CHECK(r==3&&c==7,"Fe en (3,7)"); zpos(79,&r,&c); CHECK(r==5&&c==10,"Au en (5,10)"); zpos(57,&r,&c); CHECK(r==8,"La ligne f"); zpos(72,&r,&c); CHECK(r==5&&c==3,"Hf"); }
  // 2048
  { memset(g48,0,sizeof g48); g48[0][0]=1;g48[0][1]=1;g48[0][2]=2;g48[0][3]=2; int sc=0; slide48(0,&sc); CHECK(g48[0][0]==2&&g48[0][1]==3&&g48[0][2]==0&&sc==4+8,"2048 gauche"); 
    memset(g48,0,sizeof g48); g48[0][0]=1;g48[1][0]=1;g48[2][0]=1;g48[3][0]=1; sc=0; slide48(1,&sc); CHECK(g48[0][0]==2&&g48[1][0]==2&&g48[2][0]==0,"2048 haut"); 
    memset(g48,0,sizeof g48); g48[0][0]=1;g48[0][3]=1; sc=0; slide48(3,&sc); CHECK(g48[0][3]==2&&g48[0][0]==0,"2048 droite");
    memset(g48,0,sizeof g48); for(int y=0;y<4;y++)for(int x=0;x<4;x++)g48[y][x]=1+((x+y)&1); CHECK(!can48(),"2048 bloque"); g48[2][2]=g48[2][1]; CHECK(can48(),"2048 fusion possible"); }
  // tetris : rotation, collisions, ligne
  { memset(tb,0,sizeof tb); tload(0); u8 r[4][4]; memcpy(r,tp,16); trot(r); int c=0; for(int y=0;y<4;y++)for(int x=0;x<4;x++)c+=r[y][x]; CHECK(c==4,"rotation I garde 4 cases"); CHECK(tfit(tp,3,0),"I tient"); CHECK(!tfit(tp,-1,0),"I hors mur gauche"); CHECK(!tfit(tp,7,0),"I hors mur droit"); 
    for(int x=0;x<10;x++)tb[19][x]=1; CHECK(!tfit(tp,3,19),"collision sol"); }
  // calendrier
  CHECK(dim(2,2024)==29&&dim(2,2026)==28&&dim(2,2100)==28&&dim(2,2000)==29&&dim(11,2026)==30,"jours du mois");
  printf(fails?"%d echec(s)\n":"tous les tests unitaires OK\n",fails); return fails!=0;
}
