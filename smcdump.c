#include "smc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static void tstr(unsigned t,char*o){o[0]=t>>24;o[1]=t>>16;o[2]=t>>8;o[3]=t;o[4]=0;}
int main(int argc,char**argv){
  const char *pfx = argc>1?argv[1]:"P";
  io_connect_t c=SMCOpen(); if(!c){puts("open fail");return 1;}
  int n=SMCGetKeyCount(c); printf("keys=%d\n",n);
  for(int i=0;i<n;i++){char k[5]={0}; if(SMCGetKeyFromIndex(c,i,k)!=KERN_SUCCESS)continue;
    if(strncmp(k,pfx,strlen(pfx)))continue;
    SMCKeyData_t v; if(SMCReadKey(c,k,&v)!=KERN_SUCCESS){printf("%s ERR\n",k);continue;}
    char t[5]; tstr(v.keyInfo.dataType,t);
    printf("%s %s %u ",k,t,v.keyInfo.dataSize);
    if(!strcmp(t,"flt ")){float f;memcpy(&f,v.bytes,4);printf("%.3f",f);}
    else {for(unsigned j=0;j<v.keyInfo.dataSize&&j<16;j++)printf("%02x",(unsigned char)v.bytes[j]);}
    puts("");}
}
