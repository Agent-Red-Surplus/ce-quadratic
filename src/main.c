#include <graphx.h>
#include <ti/getcsc.h>

int main(void)
{
    gfx_Begin();
    gfx_SetDrawScreen();

    gfx_FillScreen(255);

    gfx_SetColor(0);
    gfx_SetTextFGColor(0);
    gfx_SetTextBGColor(255);
    gfx_SetTextScale(1, 1);

    gfx_PrintStringXY("QGFX: GRAPHICS TEST", 16, 16);
    gfx_HorizLine(16, 34, 288);

    gfx_Rectangle(16, 50, 288, 110);

    gfx_PrintStringXY("Native graphics are running.", 28, 70);
    gfx_PrintStringXY("No logging or buffering.", 28, 94);

    gfx_Line(28, 132, 292, 132);
    gfx_Circle(160, 132, 12);

    gfx_PrintStringXY("Press any key to exit.", 16, 200);

    while (os_GetCSC()) {
    }

    while (!os_GetCSC()) {
    }

    gfx_End();
    return 0;
}
