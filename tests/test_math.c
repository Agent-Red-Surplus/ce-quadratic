#include "quadratic_math.h"
#include "factoring_math.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void reference_quadratic_solve(int32_t,int32_t,int32_t,quadratic_result_t *);
static unsigned qc,fc;
static uint32_t state=5041;
static int pick(int low,int high){state=state*1664525u+1013904223u;return low+(int)(state%(uint32_t)(high-low+1));}
static void checkq(int32_t a,int32_t b,int32_t c){
    quadratic_result_t r,v;unsigned i;quadratic_solve(a,b,c,&r);reference_quadratic_solve(a,b,c,&v);
    assert(r.count==v.count&&r.all_numbers==v.all_numbers&&r.repeated==v.repeated);
    assert(r.all_numbers==(!a&&!b&&!c));
    for(i=0;i<2;++i){root_t u=r.roots[i],w=v.roots[i];
        assert(u.p==w.p&&u.q==w.q&&u.f==w.f&&u.r==w.r&&u.radical==w.radical&&u.imaginary==w.imaginary);}
    if(!a)assert(r.count==(b?1u:0u));else{int64_t d=(int64_t)b*b-4LL*a*c;assert(r.count==(d?2u:1u)&&r.repeated==(!d));}
    for(i=0;i<r.count;++i){root_t u=r.roots[i];int64_t p=u.p,q=u.q,f=u.radical?u.f:0,s=u.radical?(u.imaginary?-u.r:u.r):0;
        assert(q>0);assert((int64_t)a*(p*p+f*f*s)+(int64_t)b*p*q+(int64_t)c*q*q==0);
        assert(f*(2LL*a*p+(int64_t)b*q)==0);}
    ++qc;
}
static const factoring_result_t *checkf(const factoring_poly_t *a){
    const factoring_result_t *r=factoring_solve(a,0);factoring_poly_t p={0},next;unsigned k;int i,j,d=a->degree;
    assert(r->valid_input&&r->verified);p.c[0]=r->scalar;
    for(k=0;k<r->count;++k){memset(&next,0,sizeof(next));next.degree=p.degree+r->factor[k].degree;
        assert(next.degree<=12);if(!r->partial)assert(r->factor[k].degree<=3);
        for(i=0;i<=p.degree;++i)for(j=0;j<=r->factor[k].degree;++j)next.c[i+j]+=p.c[i]*r->factor[k].c[j];p=next;}
    while(p.degree>0&&!p.c[p.degree])--p.degree;while(d>0&&!a->c[d])--d;assert(p.degree==d);
    for(i=0;i<=d;++i)assert(p.c[i]==a->c[i]);++fc;return r;
}
static bool cancel_now(void){return true;}
int main(void){
    int a,b,c,t,i,j;factoring_poly_t p;const factoring_result_t *r;
#ifdef TEST_FORCED_CANCEL
    memset(&p,0,sizeof(p));p.degree=12;p.c[12]=997;p.c[1]=991;p.c[0]=983;
    r=factoring_solve(&p,cancel_now);assert(r->cancelled&&r->partial&&r->verified);puts("Forced cancellation passed.");return 0;
#endif
#ifdef TEST_FORCED_BUDGET
    memset(&p,0,sizeof(p));p.degree=4;p.c[4]=1;p.c[0]=4;
    r=factoring_solve(&p,0);assert(r->limited&&r->partial&&r->verified);puts("Forced budget passed.");return 0;
#endif
    for(a=-12;a<=12;++a)for(b=-12;b<=12;++b)for(c=-12;c<=12;++c)checkq(a,b,c);
    for(t=0;t<1000;++t){a=pick(-1000,1000);b=pick(-1000,1000);c=pick(-1000,1000);checkq(a,b,c);}
    memset(&p,0,sizeof(p));p.degree=2;p.c[2]=6;p.c[1]=15;p.c[0]=6;r=checkf(&p);assert(!r->partial&&r->scalar==3&&r->count==2);
    memset(&p,0,sizeof(p));p.degree=6;p.c[6]=1;p.c[3]=5;p.c[0]=6;r=checkf(&p);assert(!r->partial&&r->count==2);
    memset(&p,0,sizeof(p));p.degree=4;p.c[4]=1;p.c[2]=5;p.c[0]=4;r=checkf(&p);assert(!r->partial&&r->count==2);
    memset(&p,0,sizeof(p));p.degree=4;p.c[4]=1;p.c[0]=4;r=checkf(&p);assert(r->partial);
    memset(&p,0,sizeof(p));p.degree=4;p.c[4]=1;p.c[3]=-8;p.c[2]=24;p.c[1]=-32;p.c[0]=16;r=checkf(&p);assert(!r->partial&&r->count==4);
    memset(&p,0,sizeof(p));p.degree=8;p.c[8]=1;p.c[0]=-1;r=checkf(&p);assert(r->partial&&r->count==4);
    memset(&p,0,sizeof(p));p.degree=12;p.c[12]=3;r=checkf(&p);assert(!r->partial&&r->count==12);
    memset(&p,0,sizeof(p));checkf(&p);p.c[0]=-7;checkf(&p);
    memset(&p,0,sizeof(p));p.degree=12;p.c[0]=5;checkf(&p);
    memset(&p,0,sizeof(p));p.degree=12;p.c[12]=1000;p.c[1]=997;p.c[0]=991;r=checkf(&p);assert(r->limited&&r->partial);
    for(t=0;t<250;++t){memset(&p,0,sizeof(p));p.degree=pick(0,12);for(i=0;i<=p.degree;++i)p.c[i]=pick(-15,15);checkf(&p);}
    for(t=0;t<150;++t){factoring_poly_t next;memset(&p,0,sizeof(p));p.c[0]=pick(1,5)*(pick(0,1)?1:-1);
        for(j=0;j<3;++j){int k=pick(1,3),lead=pick(1,3),constant=pick(1,3)*(pick(0,1)?1:-1);
            memset(&next,0,sizeof(next));next.degree=p.degree+k;
            for(i=0;i<=p.degree;++i){next.c[i]+=p.c[i]*constant;next.c[i+k]+=p.c[i]*lead;}p=next;}checkf(&p);}
    memset(&p,0,sizeof(p));p.degree=13;r=factoring_solve(&p,0);assert(!r->valid_input);
    p.degree=-1;r=factoring_solve(&p,0);assert(!r->valid_input);
    p.degree=0;p.c[0]=1001;r=factoring_solve(&p,0);assert(!r->valid_input);
    r=factoring_solve(0,0);assert(!r->valid_input);(void)cancel_now;
    printf("Quadratic cases: %u\nFactoring cases: %u\nInput rejections: 4\n",qc,fc);return 0;
}
