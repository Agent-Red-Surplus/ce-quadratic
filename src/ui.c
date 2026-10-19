#include "ui.h"
#include "app.h"
#include <graphx.h>
#include <ti/getcsc.h>
#include <string.h>
uint8_t ui_wait_key(void){uint8_t k;do{k=os_GetCSC();}while(!k);return k;}

void ui_begin(void){gfx_Begin();gfx_SetDrawBuffer();gfx_SetColor(0);gfx_SetTextFGColor(0);gfx_SetTextBGColor(255);gfx_SetTextScale(1,1);}

void ui_page(const char *s){gfx_SetTextScale(1,1);gfx_FillScreen(255);gfx_SetColor(0);gfx_PrintStringXY(s,12,12);gfx_HorizLine(12,28,296);}

bool ui_parse_number(const char *s,int32_t *v){int32_t m=0;unsigned i=0;bool neg=false;
    if(!s[0]){*v=0;return true;}if(s[0]=='-'){neg=true;i=1;if(!s[i])return false;}
    for(;s[i];++i){if(s[i]<'0'||s[i]>'9')return false;m=m*10L+s[i]-'0';if(m>APP_COEFFICIENT_LIMIT)return false;}
    *v=neg?-m:m;return true;
}

int ui_digit_key(uint8_t k){switch(k){case sk_0:return 0;case sk_1:return 1;case sk_2:return 2;case sk_3:return 3;case sk_4:return 4;case sk_5:return 5;case sk_6:return 6;case sk_7:return 7;case sk_8:return 8;case sk_9:return 9;default:return -1;}}
void ui_wrap(const char *s,int y,int last_y){
    while(*s&&y<=last_y){char line[96];size_t n=0;
        while(s[n]&&n<sizeof(line)-1){line[n]=s[n];line[n+1]='\0';if(gfx_GetStringWidth(line)>292)break;++n;}
        if(!n)break;memcpy(line,s,n);line[n]='\0';gfx_PrintStringXY(line,12,y);s+=n;y+=15;
    }
}
