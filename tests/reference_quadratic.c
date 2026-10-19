#include "quadratic_math.h"
#include <string.h>
int32_t ref_abs32(int32_t v){return v<0?-v:v;}

int32_t ref_gcd32(int32_t a,int32_t b){int32_t t;a=ref_abs32(a);b=ref_abs32(b);while(b){t=a%b;a=b;b=t;}return a;}

root_t ref_rational_root(int32_t p,int32_t q){root_t v={0};int32_t g;if(q<0){p=-p;q=-q;}g=ref_gcd32(p,q);v.p=p/g;v.q=q/g;return v;}

void reference_quadratic_solve(int32_t a,int32_t b,int32_t c,quadratic_result_t *out){
    int32_t d,m,s=0,f=1,r,i,p,q,g;
    memset(out->roots,0,sizeof(out->roots));out->count=0;out->all_numbers=false;out->repeated=false;
    if(!a){if(!b)out->all_numbers=!c;else{out->roots[0]=ref_rational_root(-c,b);out->count=1;}return;}
    d=b*b-4L*a*c;m=ref_abs32(d);while((s+1)*(s+1)<=m)++s;
    if(d>=0&&s*s==d){out->roots[0]=ref_rational_root(-b+s,2L*a);out->count=1;
        if(!s)out->repeated=true;else{out->roots[1]=ref_rational_root(-b-s,2L*a);out->count=2;}return;}
    r=m;for(i=2;i*i<=r;++i)while(r%(i*i)==0){r/=i*i;f*=i;}
    p=-b;q=2L*a;if(q<0){p=-p;q=-q;}g=ref_gcd32(ref_gcd32(p,f),q);
    out->roots[0].p=p/g;out->roots[0].q=q/g;out->roots[0].f=f/g;out->roots[0].r=r;
    out->roots[0].imaginary=d<0;out->roots[0].radical=true;out->roots[1]=out->roots[0];out->roots[1].f=-out->roots[1].f;out->count=2;
}
