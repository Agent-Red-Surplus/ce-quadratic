#include "factoring.h"
#include "factoring_math.h"
#include "ui.h"
#include <graphx.h>
#include <ti/getcsc.h>
#include <stdio.h>
#include <string.h>
static char fields[FACTOR_MAX_DEGREE+1][8];
static unsigned input_degree=2;
static bool poll_cancel(void){uint8_t k=os_GetCSC();return k==sk_Clear||k==sk_Yequ;}
static void polynomial_text(const factoring_poly_t *p,char *s,size_t size){
    int i;size_t used=0;char term[80];s[0]='\0';
    for(i=p->degree;i>=0;--i){int64_t v=p->c[i],mag=v<0?-v:v;if(!v)continue;
        if(!i)snprintf(term,sizeof(term),"%s%ld",v<0?"-":(used?"+":""),(long)mag);
        else{char coeff[32]="",power[16]="";
            if(mag!=1)snprintf(coeff,sizeof(coeff),"%ld*",(long)mag);
            if(i!=1)snprintf(power,sizeof(power),"^%d",i);
            snprintf(term,sizeof(term),"%s%sx%s",v<0?"-":(used?"+":""),coeff,power);
        }
        if(used+strlen(term)>=size)break;strcpy(s+used,term);used+=strlen(term);
    }if(!used)snprintf(s,size,"0");
}
static bool choose_degree(void){
    for(;;){char text[48];uint8_t k;ui_page("ALGEBRA / POLYNOMIAL FACTORING");
        gfx_PrintStringXY("Factoring preview / rational factors",12,45);
        snprintf(text,sizeof(text),"Highest exponent: %u",input_degree);gfx_PrintStringXY(text,12,81);
        gfx_PrintStringXY("UP/DOWN: choose degree 0 through 12",12,112);
        gfx_PrintStringXY("Integer coefficients: -1000 to 1000",12,137);
        gfx_PrintStringXY("Negative/fractional powers: not yet",12,159);
        gfx_PrintStringXY("Unresolved factors are labeled partial.",12,181);
        gfx_HorizLine(12,205,296);gfx_PrintStringXY("ENTER: coefficients  Y=/CLEAR: menu",12,222);gfx_BlitBuffer();
        k=ui_wait_key();if(k==sk_Yequ||k==sk_Clear)return false;
        if(k==sk_Up&&input_degree<FACTOR_MAX_DEGREE)++input_degree;
        if(k==sk_Down&&input_degree)--input_degree;
        if(k==sk_Enter)return true;
    }
}
static bool input_coefficients(factoring_poly_t *p){
    unsigned selected=input_degree;bool replace=true;const char *note="Blank = 0. Missing powers need zero.";
    for(;;){unsigned top=(input_degree-selected)/5*5,row;uint8_t k;char text[72],*field;size_t len;int digit;
        ui_page("FACTORING / ENTER COEFFICIENTS");gfx_PrintStringXY(note,12,43);
        for(row=0;row<5&&top+row<=input_degree;++row){unsigned power=input_degree-top-row;int y=76+(int)row*24;
            snprintf(text,sizeof(text),"x^%u : %s",power,fields[power][0]?fields[power]:"0");
            if(power==selected){gfx_Rectangle(8,y-6,304,22);gfx_PrintStringXY(">",16,y);}gfx_PrintStringXY(text,32,y);
        }
        gfx_HorizLine(12,193,296);gfx_PrintStringXY("UP/DOWN Select  ENTER Next / factor",12,204);
        gfx_PrintStringXY("(-) Sign  DEL Erase  CLEAR Field",12,217);gfx_PrintStringXY("GRAPH Factor     Y= Back to menu",12,230);gfx_BlitBuffer();
        k=ui_wait_key();field=fields[selected];len=strlen(field);digit=ui_digit_key(k);
        if(k==sk_Yequ)return false;
        if(k==sk_Up){if(selected<input_degree)++selected;replace=true;continue;}
        if(k==sk_Down){if(selected)--selected;replace=true;continue;}
        if(k==sk_Clear){field[0]='\0';replace=false;continue;}
        if(k==sk_Del){if(len)field[len-1]='\0';replace=false;continue;}
        if(k==sk_Chs||k==sk_Sub){
            if(replace){strcpy(field,"-");replace=false;}
            else if(field[0]=='-')memmove(field,field+1,strlen(field));
            else if(len<sizeof(fields[0])-1){memmove(field+1,field,len+1);field[0]='-';}continue;
        }
        if(digit>=0){if(replace){field[0]='\0';len=0;replace=false;}
            if(len<5){field[len]=(char)('0'+digit);field[len+1]='\0';}continue;
        }
        if(k==sk_Enter&&selected){--selected;replace=true;continue;}
        if(k==sk_Graph||k==sk_Enter){unsigned i;bool valid=true;memset(p,0,sizeof(*p));p->degree=(int)input_degree;
            for(i=0;i<=input_degree;++i){int32_t v;if(!ui_parse_number(fields[i],&v)){selected=i;valid=false;break;}p->c[i]=v;}
            if(valid){while(p->degree>0&&!p->c[p->degree])--p->degree;return true;}
            note="Use -1000..1000, not a lone minus.";replace=true;
        }
    }
}
static bool show_result(const factoring_poly_t *input,const factoring_result_t *r){
    unsigned selected=0;
    for(;;){char text[320],label[64];uint8_t k;ui_page("FACTORING / RESULT");
        if(!r->valid_input||!r->verified){
            gfx_PrintStringXY("Result withheld: input / check limit",12,47);
            gfx_PrintStringXY("No factorization is certified here.",12,75);
            polynomial_text(input,text,sizeof(text));ui_wrap(text,106,193);
        }else if(!selected){
            const char *status=r->cancelled?"PARTIAL: search cancelled":r->limited?"PARTIAL: resource limit":r->partial?"PARTIAL: unresolved factors":"COMPLETE OVER RATIONALS";
            if(!input->degree&&!input->c[0])status="ZERO POLYNOMIAL";
            gfx_PrintStringXY(status,12,44);gfx_PrintStringXY("Checked: product equals your input.",12,66);
            snprintf(label,sizeof(label),"Numerical factor: %ld",(long)r->scalar);gfx_PrintStringXY(label,12,90);
            snprintf(label,sizeof(label),"Polynomial factors: %u",r->count);gfx_PrintStringXY(label,12,111);
            gfx_PrintStringXY("Multiply the number and every factor.",12,135);
            gfx_PrintStringXY("Repeated factors have separate pages.",12,155);
            gfx_PrintStringXY(!input->degree&&!input->c[0]?"Zero has no unique factorization.":"RIGHT/ENTER: factors and original",12,177);
        }else if(selected<=r->count){
            snprintf(label,sizeof(label),"Factor %u of %u",selected,r->count);gfx_PrintStringXY(label,12,45);
            gfx_PrintStringXY("This is ONE factor of the product.",12,67);
            polynomial_text(&r->factor[selected-1],text,sizeof(text));ui_wrap(text,101,193);
        }else{gfx_PrintStringXY("Original polynomial",12,45);polynomial_text(input,text,sizeof(text));ui_wrap(text,80,193);}
        gfx_HorizLine(12,203,296);gfx_PrintStringXY("LEFT/RIGHT/ENTER: pages",12,215);
        gfx_PrintStringXY("CLEAR: edit     Y=: menu",12,230);gfx_BlitBuffer();k=ui_wait_key();
        if(k==sk_Yequ)return false;if(k==sk_Clear)return true;
        if(r->verified){if((k==sk_Left||k==sk_Up)&&selected)--selected;
            if(k==sk_Right||k==sk_Down||k==sk_Enter)selected=(selected+1)%(r->count+2);}
    }
}
void factoring_run(void){
    factoring_poly_t input;const factoring_result_t *r;if(!choose_degree())return;
    for(;;){if(!input_coefficients(&input))return;
        ui_page("FACTORING / SEARCHING");gfx_PrintStringXY("Bounded exact-arithmetic search",12,62);
        gfx_PrintStringXY("CLEAR / Y=: stop search",12,98);gfx_BlitBuffer();
        r=factoring_solve(&input,poll_cancel);if(!show_result(&input,r))return;
    }
}
