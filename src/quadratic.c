#include "quadratic.h"
#include "quadratic_math.h"
#include "ui.h"
#include <graphx.h>
#include <ti/getcsc.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#define CARD_MAX 96
typedef struct {
    char lead[96],radicand[32],denominator[48];
    bool radical,fraction;
    int numerator_width,width;
} expression_t;
typedef struct { char title[48],note[160],math[2][128]; } work_card_t;
static int32_t coefficient_a,coefficient_b,coefficient_c;
static char fields[3][8]={"","",""};
static root_t roots[2];
static unsigned root_count,card_count;
static bool all_numbers,repeated,card_overflow;
static work_card_t cards[CARD_MAX];






static void measure(expression_t *e){
    int w;gfx_SetTextScale(1,1);e->numerator_width=gfx_GetStringWidth(e->lead);
    if(e->radical)e->numerator_width+=17+gfx_GetStringWidth(e->radicand);
    e->width=e->numerator_width;
    if(e->fraction){w=gfx_GetStringWidth(e->denominator);if(w>e->width)e->width=w;e->width+=8;}
}

static void fraction_text(int32_t p,int32_t q,char *s,size_t n){root_t v=quad_rational_root(p,q);if(v.q==1)snprintf(s,n,"%ld",(long)v.p);else snprintf(s,n,"%ld/%ld",(long)v.p,(long)v.q);}
static void prepare_expression(const root_t *r,expression_t *e){
    char re[16]="",co[16]="";const char *sign="";memset(e,0,sizeof(*e));
    e->fraction=r->q!=1;e->radical=r->radical&&r->r!=1;
    snprintf(e->denominator,sizeof(e->denominator),"%ld",(long)r->q);
    if(!r->radical)snprintf(e->lead,sizeof(e->lead),"%ld",(long)r->p);
    else{
        if(r->p){snprintf(re,sizeof(re),"%ld",(long)r->p);sign=r->f<0?"-":"+";}else sign=r->f<0?"-":"";
        if(quad_abs32(r->f)!=1)snprintf(co,sizeof(co),"%ld",(long)quad_abs32(r->f));
        snprintf(e->lead,sizeof(e->lead),"%s%s%s%s",re,sign,co,r->imaginary?"i":"");
        if(e->radical)snprintf(e->radicand,sizeof(e->radicand),"%ld",(long)r->r);
    }measure(e);
}
static void draw_expression_scaled(const expression_t *e,int x,int middle,int scale){
    int y=e->fraction?middle-14*scale:middle;
    int nx=x+(e->width-e->numerator_width)*scale/2;
    int leadwidth,radwidth,denwidth;
    gfx_SetTextScale(1,1);leadwidth=gfx_GetStringWidth(e->lead);
    radwidth=gfx_GetStringWidth(e->radicand);denwidth=gfx_GetStringWidth(e->denominator);
    gfx_SetTextScale(scale,scale);gfx_PrintStringXY(e->lead,nx,y);
    if(e->radical){int rx=nx+leadwidth*scale;
        gfx_PrintStringXY(e->radicand,rx+12*scale,y);
        gfx_Line(rx,y+5*scale,rx+3*scale,y+8*scale);
        gfx_Line(rx+3*scale,y+8*scale,rx+8*scale,y-3*scale);
        gfx_HorizLine(rx+8*scale,y-3*scale,(radwidth+7)*scale);
    }
    if(e->fraction){gfx_HorizLine(x,middle+scale,e->width*scale);
        gfx_PrintStringXY(e->denominator,x+(e->width-denwidth)*scale/2,middle+10*scale);
    }gfx_SetTextScale(1,1);
}
static void draw_expression(const expression_t *e,int x,int y){draw_expression_scaled(e,x,y,1);}
static void root_text(const root_t *r,char *out,size_t n){
    char numerator[128];expression_t e;prepare_expression(r,&e);
    snprintf(numerator,sizeof(numerator),"%s",e.lead);
    if(e.radical){size_t used=strlen(numerator);snprintf(numerator+used,sizeof(numerator)-used,"sqrt(%s)",e.radicand);}
    if(e.fraction&&r->radical)snprintf(out,n,"(%s)/%s",numerator,e.denominator);
    else if(e.fraction)snprintf(out,n,"%s/%s",numerator,e.denominator);
    else snprintf(out,n,"%s",numerator);
}
static void solve(void){quadratic_result_t result;
    quadratic_solve(coefficient_a,coefficient_b,coefficient_c,&result);
    memcpy(roots,result.roots,sizeof(roots));root_count=result.count;
    all_numbers=result.all_numbers;repeated=result.repeated;
}


static bool input_coefficients(void){
    unsigned selected=0;bool replace=true;const char *note="Type to replace. Blank field = 0.";
    for(;;){char s[64];unsigned i;uint8_t k;char *field;size_t len;int digit;
        ui_page("ALGEBRA / QUADRATIC + LINEAR");gfx_PrintStringXY("A*x^2 + B*x + C = 0",12,42);
        gfx_PrintStringXY("Integer coefficients: -1000 to 1000",12,59);
        for(i=0;i<3;++i){int y=84+(int)i*30;const char *v=fields[i][0]?fields[i]:"0";
            snprintf(s,sizeof(s),"%c = %s",(char)('A'+i),v);
            if(i==selected){gfx_Rectangle(8,y-7,304,25);gfx_PrintStringXY(">",16,y);if(replace)gfx_HorizLine(60,y+11,gfx_GetStringWidth(v));}
            gfx_PrintStringXY(s,28,y);
        }
        gfx_PrintStringXY(note,12,173);gfx_HorizLine(12,188,296);
        gfx_PrintStringXY(selected==2?"ENTER Solve     UP/DOWN Select":"ENTER Next      UP/DOWN Select",12,199);
        gfx_PrintStringXY("DEL Erase       CLEAR Clear field",12,212);
        gfx_PrintStringXY("(-) Sign    GRAPH Solve    Y= Menu",12,225);gfx_BlitBuffer();
        k=ui_wait_key();field=fields[selected];len=strlen(field);digit=ui_digit_key(k);
        if(k==sk_Yequ)return false;
        if(k==sk_Up||k==sk_Down){selected=(selected+(k==sk_Up?2:1))%3;replace=true;note="Type to replace. Blank field = 0.";continue;}
        if(k==sk_Clear){field[0]='\0';replace=false;note="Field cleared. Blank field = 0.";continue;}
        if(k==sk_Del){if(len)field[len-1]='\0';replace=false;note="Editing: digits append to this field.";continue;}
        if(k==sk_Chs||k==sk_Sub){if(field[0]=='-')memmove(field,field+1,len);else if(len<6){memmove(field+1,field,len+1);field[0]='-';}replace=false;note="Sign changed. Type digits to append.";continue;}
        if(digit>=0){int32_t value;if(replace){field[0]='\0';len=0;replace=false;}
            if(len>=6){note="Maximum magnitude is 1000.";continue;}
            field[len]=(char)('0'+digit);field[len+1]='\0';
            if(!ui_parse_number(field,&value)){field[len]='\0';note="Maximum magnitude is 1000.";}else note="Editing: digits append to this field.";continue;}
        if(k==sk_Enter&&selected<2){++selected;replace=true;note="Type to replace. Blank field = 0.";continue;}
        if(k==sk_Enter||k==sk_Graph){int32_t v[3];for(i=0;i<3;++i)if(!ui_parse_number(fields[i],&v[i]))break;
            if(i!=3){selected=i;replace=false;note="Enter digits after the minus sign.";continue;}
            coefficient_a=v[0];coefficient_b=v[1];coefficient_c=v[2];return true;
        }
    }
}

static void new_card(const char *title,const char *note){
    if(card_count>=CARD_MAX){card_overflow=true;return;}
    memset(&cards[card_count],0,sizeof(cards[card_count]));
    snprintf(cards[card_count].title,sizeof(cards[card_count].title),"%s",title);
    snprintf(cards[card_count].note,sizeof(cards[card_count].note),"%s",note);++card_count;
}
static void math_row(unsigned row,const char *format,...){
    va_list args;int result;if(!card_count||row>1)return;
    va_start(args,format);result=vsnprintf(cards[card_count-1].math[row],sizeof(cards[card_count-1].math[row]),format,args);va_end(args);
    if(result<0||result>=(int)sizeof(cards[card_count-1].math[row]))card_overflow=true;
}
static void fraction_card(const char *title,const char *name,int32_t p,int32_t q){
    int32_t g;char value[40],note[120];
    if(q<0){new_card(title,"Make the denominator positive by changing both signs.");
        math_row(0,"%s=(%ld)/(%ld)",name,(long)p,(long)q);p=-p;q=-q;
        math_row(1,"%s=(%ld)/(%ld)",name,(long)p,(long)q);}
    g=quad_gcd32(p,q);fraction_text(p,q,value,sizeof(value));
    if(g>1)snprintf(note,sizeof(note),"Divide the top and bottom by %ld.",(long)g);
    else snprintf(note,sizeof(note),"No common factor remains. This is already simplified.");
    new_card(title,note);math_row(0,"%s=(%ld)/(%ld)",name,(long)p,(long)q);math_row(1,"%s=%s",name,value);
}
static void format_equation(char *out,size_t size){
    int32_t v[3]={coefficient_a,coefficient_b,coefficient_c};
    const char *power[3]={"x^2","x",""};unsigned i;bool first=true;out[0]='\0';
    for(i=0;i<3;++i){char term[40],number[16]="";size_t n;if(!v[i])continue;
        if(i==2||quad_abs32(v[i])!=1)snprintf(number,sizeof(number),"%ld",(long)quad_abs32(v[i]));
        snprintf(term,sizeof(term),"%s%s%s",first?(v[i]<0?"-":""):(v[i]<0?" - ":" + "),number,power[i]);
        n=strlen(out);snprintf(out+n,size-n,"%s",term);first=false;
    }
    if(first)snprintf(out,size,"0");
    {size_t n=strlen(out);snprintf(out+n,size-n," = 0");}
}
static void build_work(void){
    int32_t a=coefficient_a,b=coefficient_b,c=coefficient_c,d,m,s=0,f=1,r,i,p,q,g;
    unsigned j;char equation[128],value[128],note[160],h[40],k[40];
    card_count=0;card_overflow=false;format_equation(equation,sizeof(equation));
    new_card("Start with the equation","Identify the coefficients before substituting.");
    math_row(0,"%s",equation);math_row(1,"A=%ld, B=%ld, C=%ld",(long)a,(long)b,(long)c);
    if(!a){
        if(!b){new_card(!c?"Every value works":"There is no solution",!c?"This statement is always true.":"This statement is false, whatever x is.");
            math_row(0,"%ld = 0",(long)c);math_row(1,!c?"All values of x":"No solution");}
        else{
            new_card("Move the constant","Subtract the constant from both sides.");
            math_row(0,"(%ld)x + (%ld) = 0",(long)b,(long)c);math_row(1,"(%ld)x = %ld",(long)b,(long)-c);
            fraction_card("Divide to isolate x","x",-c,b);
            new_card("Check the answer","Substitute the answer into the original linear equation.");
            math_row(0,"B*x+C = [(%ld)(%ld)+(%ld)(%ld)]/%ld",(long)b,(long)roots[0].p,(long)c,(long)roots[0].q,(long)roots[0].q);
            math_row(1,"B*x+C = 0");
        }
        new_card("This is not a parabola","With A=0 there is no quadratic vertex or opening direction.");math_row(0,"A = 0");return;
    }
    d=b*b-4L*a*c;m=quad_abs32(d);while((s+1)*(s+1)<=m)++s;
    new_card("Find the discriminant","Substitute into D = B^2 - 4AC.");
    math_row(0,"D = (%ld)^2 - 4(%ld)(%ld)",(long)b,(long)a,(long)c);
    math_row(1,"D = %ld - (%ld)",(long)(b*b),(long)(4L*a*c));
    new_card("Simplify the discriminant",d<0?"A negative discriminant gives two nonreal complex roots.":(!d?"Zero means the two roots will be the same.":"A positive discriminant gives two distinct real roots."));
    math_row(0,"D = %ld - (%ld)",(long)(b*b),(long)(4L*a*c));math_row(1,"D = %ld",(long)d);
    new_card("Use the quadratic formula","The plus/minus sign represents two cases.");
    math_row(0,"x = (-B+/-sqrt(D))/(2A)");
    math_row(1,"x = (%ld+/-sqrt(%ld))/(%ld)",(long)-b,(long)d,(long)(2L*a));
    if(d>=0&&s*s==d){
        snprintf(note,sizeof(note),"%ld squared is %ld, so its square root is %ld.",(long)s,(long)d,(long)s);
        new_card("Simplify the square root",note);math_row(0,"sqrt(%ld)",(long)d);math_row(1,"= %ld",(long)s);
        for(j=0;j<(s?2U:1U);++j){const char *name=j?"x2":"x1";int32_t n=-b+(j?-s:s);
            new_card(j?"Take the minus case":"Take the plus case","Simplify the numerator, then reduce the fraction.");
            math_row(0,"%s = (%ld%s%ld)/(%ld)",name,(long)-b,j?"-":"+",(long)s,(long)(2L*a));
            math_row(1,"%s = (%ld)/(%ld)",name,(long)n,(long)(2L*a));fraction_card("Reduce the answer",name,n,2L*a);
        }
        if(!s){new_card("A repeated root","Adding or subtracting zero gives the same answer.");root_text(&roots[0],value,sizeof(value));math_row(0,"x = %s",value);}
    }else{
        if(d<0){new_card("Take out the imaginary unit","Use sqrt(-1)=i, since i^2=-1.");math_row(0,"sqrt(%ld)",(long)d);math_row(1,"= isqrt(%ld)",(long)m);}
        r=m;
        for(i=2;i*i<=r;++i)while(r%(i*i)==0){int32_t old=r;r/=i*i;f*=i;
            snprintf(note,sizeof(note),"Take %ld outside the square root.",(long)i);
            new_card("Pull out a square factor",note);math_row(0,"%ld = %ld^2 * %ld",(long)old,(long)i,(long)r);
            math_row(1,"sqrt(%ld) = %ldsqrt(%ld)",(long)m,(long)f,(long)r);
        }
        if(r==1){new_card("The remaining root is 1","Since sqrt(1)=1, no radical remains.");math_row(0,"sqrt(%ld) = %ld",(long)m,(long)f);}
        else{new_card("The radical is simplified","No perfect-square factor greater than 1 remains inside.");math_row(0,"sqrt(%ld) = %ldsqrt(%ld)",(long)m,(long)f,(long)r);}
        p=-b;q=2L*a;
        new_card("Put it back into the formula","Keep the entire numerator above the fraction bar.");
        if(r==1)math_row(0,"x = (%ld+/-%ldi)/(%ld)",(long)p,(long)f,(long)q);
        else math_row(0,"x = (%ld+/-%ld%ssqrt(%ld))/(%ld)",(long)p,(long)f,d<0?"i":"",(long)r,(long)q);
        if(q<0){p=-p;q=-q;new_card("Make the denominator positive","Changing every sign swaps the two cases, not the answer set.");
            if(r==1)math_row(0,"x = (%ld+/-%ldi)/%ld",(long)p,(long)f,(long)q);
            else math_row(0,"x = (%ld+/-%ld%ssqrt(%ld))/%ld",(long)p,(long)f,d<0?"i":"",(long)r,(long)q);
        }
        g=quad_gcd32(quad_gcd32(p,f),q);
        if(g>1){snprintf(note,sizeof(note),"Divide both numerator terms and the denominator by %ld.",(long)g);
            new_card("Reduce the whole fraction",note);math_row(0,"%ld/%ld = %ld",(long)p,(long)g,(long)(p/g));
            math_row(1,"%ld/%ld = %ld",(long)f,(long)g,(long)(f/g));
            new_card("Reduce the denominator","Divide the bottom by that same common factor.");
            math_row(0,"%ld/%ld = %ld",(long)q,(long)g,(long)(q/g));}
        new_card("Write the two answers","Use the plus sign for one answer and the minus sign for the other.");
        for(j=0;j<2;++j){root_text(&roots[j],value,sizeof(value));math_row(j,"x%u = %s",j+1,value);}
    }
    {
        int32_t sp,sq,pp,pq;char sum[40],prod[40],targetsum[40],targetprod[40];
        if(roots[0].radical){int32_t sign=roots[0].imaginary?-1L:1L;
            sp=2L*roots[0].p;sq=roots[0].q;
            pp=roots[0].p*roots[0].p-sign*roots[0].f*roots[0].f*roots[0].r;pq=sq*sq;
            new_card("Check conjugate roots","The added terms cancel in the sum. Use i^2=-1 for imaginary terms.");
            math_row(0,"(u+v)+(u-v) = 2u");math_row(1,"(u+v)(u-v) = u^2-v^2");
        }else{const root_t *r1=&roots[0],*r2=repeated?&roots[0]:&roots[1];
            sp=r1->p*r2->q+r2->p*r1->q;sq=r1->q*r2->q;pp=r1->p*r2->p;pq=sq;
        }
        fraction_text(sp,sq,sum,sizeof(sum));fraction_text(pp,pq,prod,sizeof(prod));
        fraction_text(-b,a,targetsum,sizeof(targetsum));fraction_text(c,a,targetprod,sizeof(targetprod));
        new_card("Add the roots","Combine the terms, then reduce the resulting fraction.");
        math_row(0,"x1+x2 = (%ld)/(%ld)",(long)sp,(long)sq);math_row(1,"x1+x2 = %s",sum);
        new_card("Check the sum","This must equal -B/A. Count a repeated root twice.");
        math_row(0,"-B/A = (%ld)/(%ld)",(long)-b,(long)a);math_row(1,"-B/A = %s",targetsum);
        new_card("Multiply the roots","Multiply the terms, then reduce the resulting fraction.");
        math_row(0,"x1*x2 = (%ld)/(%ld)",(long)pp,(long)pq);math_row(1,"x1*x2 = %s",prod);
        new_card("Check the product","This must equal C/A.");math_row(0,"C/A = (%ld)/(%ld)",(long)c,(long)a);math_row(1,"C/A = %s",targetprod);
        if(strcmp(sum,targetsum)||strcmp(prod,targetprod)){new_card("Check failed","Do not rely on these answers. Return to the menu.");}
    }
    new_card("Now find the vertex","For the associated parabola, first find its horizontal coordinate.");
    math_row(0,"h = (-B)/(2A)");math_row(1,"h = (%ld)/(%ld)",(long)-b,(long)(2L*a));
    fraction_card("Simplify h","h",-b,2L*a);
    new_card("Find the vertical coordinate","Use the discriminant already calculated above.");
    math_row(0,"k = (-D)/(4A)");math_row(1,"k = (%ld)/(%ld)",(long)-d,(long)(4L*a));
    fraction_card("Simplify k","k",-d,4L*a);
    fraction_text(-b,2L*a,h,sizeof(h));fraction_text(-d,4L*a,k,sizeof(k));
    new_card("Vertex and symmetry axis","The axis is the vertical line through the vertex.");
    math_row(0,"Vertex: (%s, %s)",h,k);math_row(1,"Axis: x = %s",h);
    new_card(a>0?"Opens upward":"Opens downward",a>0?"A is positive, so the vertex is the lowest point.":"A is negative, so the vertex is the highest point.");
    math_row(0,"A = %ld",(long)a);math_row(1,a>0?"Minimum y = %s":"Maximum y = %s",k);
}

static void trim(char *s){
    size_t n=strlen(s),start=0;while(s[start]==' ')++start;
    if(start){memmove(s,s+start,n-start+1);n-=start;}
    while(n&&s[n-1]==' ')s[--n]='\0';
}
static void strip_outer(char *s){
    size_t n;bool again=true;trim(s);
    while(again){int depth=0;size_t i;n=strlen(s);again=false;
        if(n<2||s[0]!='('||s[n-1]!=')')return;
        for(i=0;i<n;++i){if(s[i]=='(')++depth;else if(s[i]==')')--depth;if(!depth&&i<n-1)return;}
        memmove(s,s+1,n-2);s[n-2]='\0';trim(s);again=true;
    }
}
static void parse_side(const char *s,expression_t *e){
    char number[128];size_t i;int depth=0;char *rad,*end;
    memset(e,0,sizeof(*e));snprintf(number,sizeof(number),"%s",s);trim(number);
    for(i=0;number[i];++i){
        if(number[i]=='(')++depth;else if(number[i]==')')--depth;
        else if(number[i]=='/'&&!depth&&!(i&&number[i-1]=='+'&&number[i+1]=='-')){
            snprintf(e->denominator,sizeof(e->denominator),"%s",number+i+1);
            strip_outer(e->denominator);number[i]='\0';e->fraction=true;break;
        }
    }
    strip_outer(number);rad=strstr(number,"sqrt(");
    if(rad){end=strchr(rad+5,')');
        if(end&&end[1]=='\0'){
            size_t n=(size_t)(end-(rad+5));if(n>=sizeof(e->radicand))n=sizeof(e->radicand)-1;
            memcpy(e->radicand,rad+5,n);e->radicand[n]='\0';*rad='\0';trim(number);
            n=strlen(number);if(n&&number[n-1]=='*')number[n-1]='\0';
            e->radical=true;
        }
    }
    snprintf(e->lead,sizeof(e->lead),"%.95s",number);measure(e);
}
static void draw_math_row(const char *line,int middle){
    char left[128],right[128];const char *eq=strchr(line,'=');
    expression_t l,r;int width,scale,x=16;bool has_eq=eq!=NULL;
    if(!line[0])return;
    if(has_eq){size_t n=(size_t)(eq-line);if(n>=sizeof(left))n=sizeof(left)-1;memcpy(left,line,n);left[n]='\0';snprintf(right,sizeof(right),"%s",eq+1);}
    else{snprintf(left,sizeof(left),"%s",line);right[0]='\0';}
    parse_side(left,&l);parse_side(right,&r);
    width=l.width+(has_eq?16+r.width:0);
    scale=width*2<=288?2:1;
    if(width>288){
        char part[80];size_t at=0,n=0;int y=middle-10;
        while(line[at]&&y<=middle+14){n=0;
            while(line[at+n]&&n<sizeof(part)-1){part[n]=line[at+n];part[n+1]='\0';if(gfx_GetStringWidth(part)>288)break;++n;}
            if(!n)break;
            memcpy(part,line+at,n);part[n]='\0';gfx_PrintStringXY(part,16,y);at+=n;y+=12;
        }return;
    }
    draw_expression_scaled(&l,x,middle,scale);x+=l.width*scale;
    if(has_eq){gfx_SetTextScale(scale,scale);gfx_PrintStringXY("=",x+3*scale,middle);gfx_SetTextScale(1,1);x+=16*scale;draw_expression_scaled(&r,x,middle,scale);}
}
static void draw_note(const char *note){
    unsigned row;const char *p=note;
    for(row=0;row<3&&*p;++row){char line[128];size_t n=0,last=0,take;
        while(*p==' ')++p;
        while(p[n]&&n<sizeof(line)-1){line[n]=p[n];line[n+1]='\0';if(gfx_GetStringWidth(line)>292)break;if(p[n]==' ')last=n;++n;}
        if(!n)break;
        take=p[n]&&last?last:n;memcpy(line,p,take);line[take]='\0';gfx_PrintStringXY(line,12,54+(int)row*12);p+=take;
    }
}
static bool show_work(void){
    unsigned selected=0;
    for(;;){char count[40];uint8_t k;work_card_t *card=&cards[selected];
        ui_page(card->title);snprintf(count,sizeof(count),"Step %u of %u",selected+1,card_count);
        gfx_PrintStringXY(count,12,39);gfx_PrintStringXY("Y= Menu",250,39);draw_note(card->note);
        draw_math_row(card->math[0],118);draw_math_row(card->math[1],183);
        gfx_HorizLine(12,221,296);
        gfx_PrintStringXY(card_overflow?"Work limit reached: view incomplete":
            (selected+1<card_count?"UP Back  ENTER Next  GRAPH Roots":"UP Back  ENTER Roots GRAPH Roots"),12,231);
        gfx_BlitBuffer();k=ui_wait_key();
        if(k==sk_Yequ)return false;if(k==sk_Clear||k==sk_Graph)return true;
        if(k==sk_Up||k==sk_Left){if(selected)--selected;}
        else if(k==sk_Down||k==sk_Right){if(selected+1<card_count)++selected;}
        else if(k==sk_Enter){if(selected+1<card_count)++selected;else return true;}
    }
}
static bool show_details(void){
    int32_t a=coefficient_a,b=coefficient_b,c=coefficient_c;
    for(;;){char text[80];uint8_t key;ui_page("ALGEBRA / EQUATION DETAILS");
        snprintf(text,sizeof(text),"A=%ld B=%ld C=%ld",(long)a,(long)b,(long)c);gfx_PrintStringXY(text,12,43);
        if(!a){gfx_PrintStringXY(b?"Linear equation (A=0)":"Constant equation (A=B=0)",12,66);
            gfx_PrintStringXY("Quadratic discriminant: not applicable",12,90);gfx_PrintStringXY("Parabola vertex: not applicable",12,110);
            gfx_PrintStringXY("Symmetry axis: not applicable",12,130);gfx_PrintStringXY("Opening/min/max: not applicable",12,150);
            gfx_PrintStringXY(b?"One rational solution":(!c?"Identity: every x is a solution":"Contradiction: no solution"),12,178);
        }else{int32_t d=b*b-4L*a*c;root_t h=quad_rational_root(-b,2L*a),v=quad_rational_root(-d,4L*a);expression_t he,ve;int x;
            prepare_expression(&h,&he);prepare_expression(&v,&ve);snprintf(text,sizeof(text),"D = B^2 - 4AC = %ld",(long)d);gfx_PrintStringXY(text,12,64);
            gfx_PrintStringXY(d<0?"Two nonreal complex roots":(!d?"One repeated real root":(roots[0].radical?"Two irrational real roots":"Two rational real roots")),12,82);
            gfx_PrintStringXY("Vertex:",12,118);gfx_PrintStringXY("(",72,118);x=84;draw_expression(&he,x,118);x+=he.width+5;gfx_PrintStringXY(",",x,118);x+=14;draw_expression(&ve,x,118);x+=ve.width+5;gfx_PrintStringXY(")",x,118);
            gfx_PrintStringXY("Axis: x =",12,162);draw_expression(&he,90,162);gfx_PrintStringXY(a>0?"Opens up; vertex is a minimum.":"Opens down; vertex is a maximum.",12,192);
        }
        gfx_HorizLine(12,207,296);gfx_PrintStringXY("ENTER/CLEAR/GRAPH: roots",12,218);gfx_PrintStringXY("TRACE: work       Y=: main menu",12,231);gfx_BlitBuffer();
        key=ui_wait_key();if(key==sk_Enter||key==sk_Clear||key==sk_Graph)return true;if(key==sk_Yequ)return false;if(key==sk_Trace)return show_work();
    }
}
static bool show_results(void){
    expression_t e[2];unsigned i;bool fits=true;
    for(i=0;i<root_count;++i){prepare_expression(&roots[i],&e[i]);if(e[i].width>296)fits=false;}
    for(;;){char text[64];uint8_t k;ui_page("ALGEBRA / EXACT SOLUTIONS");
        snprintf(text,sizeof(text),"A=%ld B=%ld C=%ld",(long)coefficient_a,(long)coefficient_b,(long)coefficient_c);gfx_PrintStringXY(text,12,44);
        if(!fits)gfx_PrintStringXY("Answer exceeds display width.",12,88);
        else if(!root_count)gfx_PrintStringXY(all_numbers?"ALL COMPLEX NUMBERS":"NO SOLUTION",12,88);
        else if(root_count==1){draw_expression(&e[0],12,96);if(repeated)gfx_PrintStringXY("Repeated root",12,140);}
        else{int combined=e[0].width+e[1].width+24;
            if(combined<=296){int x=12;draw_expression(&e[0],x,100);x+=e[0].width+6;gfx_PrintStringXY(",",x,100);draw_expression(&e[1],x+18,100);}
            else{draw_expression(&e[0],12,86);if(e[0].width<=282)gfx_PrintStringXY(",",18+e[0].width,86);else gfx_PrintStringXY(",",12,116);draw_expression(&e[1],12,150);}
        }
        gfx_HorizLine(12,195,296);gfx_PrintStringXY("ENTER/CLEAR: edit coefficients",12,207);gfx_PrintStringXY("GRAPH: details   TRACE: work",12,220);gfx_PrintStringXY("Y=: main menu",12,232);gfx_BlitBuffer();
        k=ui_wait_key();if(k==sk_Enter||k==sk_Clear)return true;if(k==sk_Yequ)return false;
        if(k==sk_Graph&&!show_details())return false;if(k==sk_Trace&&!show_work())return false;
    }
}
void quadratic_run(void){for(;;){if(!input_coefficients())return;solve();build_work();if(!show_work())return;if(!show_results())return;}}
