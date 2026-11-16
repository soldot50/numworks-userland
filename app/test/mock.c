#define _POSIX_C_SOURCE 200809L
#include "eadk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static FILE*ops; static uint64_t T=0; static uint32_t seed=12345;
static char toks[6000][32]; static int nt=0, ti=0, phase=0, cnt=0, tail=0;
static void init(void){ static int done=0; if(done)return; done=1; ops=fopen(getenv("OPS")?getenv("OPS"):"ops.txt","w");
  const char*k=getenv("KEYS"); char*c=strdup(k?k:""); for(char*t=strtok(c,",");t&&nt<6000;t=strtok(0,","))strcpy(toks[nt++],t);
  const char*s=getenv("SEED"); if(s)seed=atoi(s);}
void eadk_display_push_rect_uniform(eadk_rect_t r,eadk_color_t c){init();fprintf(ops,"R %d %d %d %d %d\n",r.x,r.y,r.width,r.height,c);}
void eadk_display_draw_string(const char*t,eadk_point_t p,bool l,eadk_color_t f,eadk_color_t b){init();fprintf(ops,"T %d %d %d %d %d %s\n",p.x,p.y,l,f,b,t);}
uint64_t eadk_timing_millis(void){return T;}
void eadk_timing_msleep(uint32_t ms){T+=ms;}
uint32_t eadk_random(void){seed=seed*1103515245u+12345u;return seed>>8;}
void eadk_backlight_set_brightness(uint8_t b){init();fprintf(ops,"B %d\n",b);}
static int keyof(const char*t){ if(!strcmp(t,"L"))return 0; if(!strcmp(t,"U"))return 1; if(!strcmp(t,"D"))return 2; if(!strcmp(t,"R"))return 3; if(!strcmp(t,"OK"))return 4; if(!strcmp(t,"B"))return 5; if(!strcmp(t,"BK"))return 17; if(!strcmp(t,"EXE"))return 52; if(t[0]=='k')return atoi(t+1); return -1;}
eadk_keyboard_state_t eadk_keyboard_scan(void){ init(); T+=1;
  for(;;){
    if(ti>=nt){ if(tail++>400){fclose(ops);exit(0);} return (tail%6<3)?(1ULL<<5):0; }
    char*t=toks[ti];
    if(!strncmp(t,"SNAP:",5)){fprintf(ops,"S %s\n",t+5);ti++;continue;}
    if(t[0]=='W'){ if(cnt++<atoi(t+1))return 0; cnt=0;ti++;continue;}
    if(t[0]=='H'){ // H<key>:<n> maintenir
      char*col=strchr(t,':'); int n=atoi(col+1); char kk[16]; strncpy(kk,t+1,col-t-1);kk[col-t-1]=0; int k=keyof(kk);
      if(cnt++<n)return 1ULL<<k; cnt=0;ti++;continue;}
    int k=keyof(t); if(k<0){ti++;continue;}
    if(phase==0){ if(cnt++<3)return 1ULL<<k; phase=1;cnt=0;}
    if(phase==1){ if(cnt++<3)return 0; phase=0;cnt=0;ti++;continue;}
  }}
