#include "factoring_math.h"
#include "app.h"
#include <string.h>
#ifndef FACTOR_WORK_BUDGET
#define FACTOR_WORK_BUDGET 50000UL
#endif
#ifndef FACTOR_POLL_MASK
#define FACTOR_POLL_MASK 255UL
#endif
static factoring_result_t result;
static bool (*cancel_callback)(void);
static int64_t magnitude(int64_t x){return x<0?-x:x;}
static int64_t gcd64(int64_t a,int64_t b){int64_t t;a=magnitude(a);b=magnitude(b);while(b){t=a%b;a=b;b=t;}return a;}
static void trim_poly(factoring_poly_t *p){while(p->degree>0&&!p->c[p->degree])--p->degree;}
static bool tick(void){
    if(result.cancelled||result.limited)return false;
    if(++result.work>FACTOR_WORK_BUDGET){result.limited=true;return false;}
    if((result.work&FACTOR_POLL_MASK)==0&&cancel_callback&&cancel_callback()){result.cancelled=true;return false;}
    return true;
}
static void store_factor(const factoring_poly_t *p){
    if(result.count<FACTOR_MAX_FACTORS)result.factor[result.count++]=*p;
    else{result.partial=true;result.limited=true;}
}
static int divisors(int64_t value,int64_t *out){
    int count=0;int64_t d;value=magnitude(value);
    for(d=1;d<=value/d;++d){
        if(!tick())return -1;
        if(value%d==0){if(count>=254){result.limited=true;return -1;}
            out[count++]=d;if(d!=value/d)out[count++]=value/d;}
    }return count;
}
/* Divide exactly by q*x^k-p; bounded operands keep int64_t products safe. */
static bool divide_binomial(const factoring_poly_t *a,int k,int64_t p,int64_t q,factoring_poly_t *quot){
    int i;int64_t rem[FACTOR_MAX_DEGREE+1],v;
    memset(quot,0,sizeof(*quot));quot->degree=a->degree-k;memcpy(rem,a->c,sizeof(rem));
    for(i=a->degree;i>=k;--i){
        if(rem[i]%q)return false;
        v=rem[i]/q;quot->c[i-k]=v;rem[i]=0;v=rem[i-k]+v*p;
        if(magnitude(v)>FACTOR_INTERMEDIATE_CAP){result.limited=true;return false;}rem[i-k]=v;
    }
    for(i=0;i<k;++i)if(rem[i])return false;
    trim_poly(quot);return true;
}
static void split(const factoring_poly_t *a){
    /* Recursive calls may reuse these arrays: a successful split returns immediately. */
    static int64_t pd[256],qd[256];
    int k,ip,iq,sign,np,nq;int64_t p,q;factoring_poly_t binomial,quot;
    if(a->degree<=1){store_factor(a);return;}
    if(result.limited||result.cancelled){result.partial=true;store_factor(a);return;}
    np=divisors(a->c[0],pd);nq=divisors(a->c[a->degree],qd);
    if(np<0||nq<0){result.partial=true;store_factor(a);return;}
    for(k=1;k<a->degree;++k)for(iq=0;iq<nq;++iq)for(ip=0;ip<np;++ip)for(sign=-1;sign<=1;sign+=2){
        if(!tick()){result.partial=true;store_factor(a);return;}
        p=sign*pd[ip];q=qd[iq];if(gcd64(p,q)!=1)continue;
        if(divide_binomial(a,k,p,q,&quot)){
            memset(&binomial,0,sizeof(binomial));binomial.degree=k;binomial.c[0]=-p;binomial.c[k]=q;
            split(&binomial);split(&quot);return;
        }
        if(result.limited){result.partial=true;store_factor(a);return;}
    }
    if(a->degree>=4)result.partial=true;
    store_factor(a);
}
static bool verify_product(const factoring_poly_t *original){
    factoring_poly_t product,next,normalized=*original;unsigned f;int i,j;
    trim_poly(&normalized);original=&normalized;
    memset(&product,0,sizeof(product));product.c[0]=result.scalar;
    for(f=0;f<result.count;++f){
        memset(&next,0,sizeof(next));next.degree=product.degree+result.factor[f].degree;
        if(next.degree>FACTOR_MAX_DEGREE)return false;
        for(i=0;i<=product.degree;++i)for(j=0;j<=result.factor[f].degree;++j){
            int64_t v=next.c[i+j]+product.c[i]*result.factor[f].c[j];
            if(magnitude(v)>FACTOR_INTERMEDIATE_CAP)return false;next.c[i+j]=v;
        }product=next;
    }
    trim_poly(&product);if(product.degree!=original->degree)return false;
    for(i=0;i<=product.degree;++i)if(product.c[i]!=original->c[i])return false;
    return true;
}
const factoring_result_t *factoring_solve(const factoring_poly_t *input,bool (*cancel)(void)){
    factoring_poly_t a,x;int i,low=0;int64_t g=0;
    memset(&result,0,sizeof(result));result.scalar=1;cancel_callback=cancel;
    if(!input||input->degree<0||input->degree>FACTOR_MAX_DEGREE)return &result;
    for(i=0;i<=input->degree;++i)if(input->c[i]<-APP_COEFFICIENT_LIMIT||input->c[i]>APP_COEFFICIENT_LIMIT)return &result;
    result.valid_input=true;a=*input;trim_poly(&a);
    for(i=a.degree+1;i<=FACTOR_MAX_DEGREE;++i)a.c[i]=0;
    if(a.degree==0){result.scalar=a.c[0];result.verified=verify_product(&a);return &result;}
    for(i=0;i<=a.degree;++i)g=gcd64(g,a.c[i]);
    if(a.c[a.degree]<0)g=-g;result.scalar=g;
    for(i=0;i<=a.degree;++i)a.c[i]/=g;
    while(!a.c[low])++low;
    if(low){
        memset(&x,0,sizeof(x));x.degree=1;x.c[1]=1;
        for(i=0;i<low;++i)store_factor(&x);
        for(i=0;i<=a.degree-low;++i)a.c[i]=a.c[i+low];
        for(i=a.degree-low+1;i<=a.degree;++i)a.c[i]=0;a.degree-=low;
    }
    if(a.degree)split(&a);
    result.verified=verify_product(input);return &result;
}
