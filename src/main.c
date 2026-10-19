#include "app.h"
#include "ui.h"
#include "quadratic.h"
#include "factoring.h"
#include <graphx.h>
#include <ti/getcsc.h>
static void about(void){uint8_t k;ui_page("ALGEBRA SUITE / ABOUT");
    gfx_PrintStringXY("Version " APP_VERSION,12,44);
    gfx_PrintStringXY("Quadratic / linear + factoring preview",12,75);
    gfx_PrintStringXY("Separate source files; one program",12,105);
    gfx_PrintStringXY("Integer inputs: -1000 to 1000",12,135);
    gfx_PrintStringXY("Factoring: degree 0 through 12",12,165);
    gfx_PrintStringXY("ENTER / CLEAR / Y=: menu",12,220);gfx_BlitBuffer();
    do{k=ui_wait_key();}while(k!=sk_Enter&&k!=sk_Clear&&k!=sk_Yequ);
}
static unsigned menu(unsigned selected){
    static const char *items[]={"Quadratic / linear solver","Polynomial factoring (preview)","About","Exit"};
    for(;;){unsigned i;uint8_t k;ui_page("ALGEBRA SUITE");gfx_PrintStringXY("Honors Algebra 2 project",12,44);
        for(i=0;i<4;++i){int y=76+(int)i*27;
            if(i==selected){gfx_Rectangle(8,y-8,304,25);gfx_PrintStringXY(">",16,y);}gfx_PrintStringXY(items[i],34,y);}
        gfx_PrintStringXY("Version " APP_VERSION,12,185);gfx_HorizLine(12,203,296);
        gfx_PrintStringXY("UP/DOWN: select   ENTER: open",12,214);gfx_PrintStringXY("CLEAR or Y=: quit",12,228);gfx_BlitBuffer();k=ui_wait_key();
        if(k==sk_Up)selected=(selected+3)%4;else if(k==sk_Down)selected=(selected+1)%4;
        else if(k==sk_Enter)return selected;else if(k==sk_Clear||k==sk_Yequ)return 3;
    }
}
int main(void){unsigned selected=0;bool running=true;ui_begin();
    while(running){selected=menu(selected);switch(selected){
        case 0:quadratic_run();break;case 1:factoring_run();break;case 2:about();break;default:running=false;break;}}
    gfx_End();return 0;
}
