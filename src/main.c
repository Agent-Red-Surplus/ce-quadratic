#include <ti/screen.h>
#include <ti/getcsc.h>

int main(void)
{
    os_ClrHome();

    os_PutStrFull("QCHECK: NATIVE CODE RUNNING");

    while (os_GetCSC()) {
    }

    while (!os_GetCSC()) {
    }

    return 0;
}
