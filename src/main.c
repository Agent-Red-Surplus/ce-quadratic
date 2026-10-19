#include <graphx.h>
#include <fileioc.h>
#include <ti/getcsc.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define LOG_NAME "QLTST"
#define LOG_MAGIC 0x514C5431UL

typedef struct {
    uint32_t magic;
    uint32_t launches;
    uint32_t stage;
} log_record_t;

static bool write_record(const log_record_t *record)
{
    uint8_t handle;
    bool written;
    int closed;

    handle = ti_Open(LOG_NAME, "w");

    if (!handle) {
        return false;
    }

    written = ti_Write(
        record, sizeof(*record), 1, handle
    ) == 1;

    closed = ti_Close(handle);

    return written && closed != 0;
}

static bool read_record(log_record_t *record)
{
    uint8_t handle;
    bool read_ok;
    int closed;

    handle = ti_Open(LOG_NAME, "r");

    if (!handle) {
        return false;
    }

    read_ok = ti_Read(
        record, sizeof(*record), 1, handle
    ) == 1;

    closed = ti_Close(handle);

    return read_ok && closed != 0 &&
           record->magic == LOG_MAGIC;
}

int main(void)
{
    log_record_t previous = {0};
    log_record_t current = {0};
    log_record_t verified = {0};

    bool had_previous;
    bool write_ok;
    bool verify_ok = false;
    char text[64];

    had_previous = read_record(&previous);

    current.magic = LOG_MAGIC;
    current.launches = had_previous
        ? previous.launches + 1
        : 1;
    current.stage = 10;

    write_ok = write_record(&current);

    if (write_ok && read_record(&verified)) {
        verify_ok =
            verified.launches == current.launches &&
            verified.stage == current.stage;
    }

    gfx_Begin();
    gfx_SetDrawBuffer();

    gfx_FillScreen(255);
    gfx_SetColor(0);
    gfx_SetTextFGColor(0);
    gfx_SetTextBGColor(255);
    gfx_SetTextScale(1, 1);

    gfx_PrintStringXY("QLOG: PERSISTENCE TEST", 16, 16);
    gfx_HorizLine(16, 34, 288);

    snprintf(
        text, sizeof(text),
        "Launch count: %lu",
        (unsigned long)current.launches
    );
    gfx_PrintStringXY(text, 16, 54);

    gfx_PrintStringXY(
        had_previous
            ? "Previous record: loaded"
            : "Previous record: none valid",
        16, 78
    );

    if (had_previous) {
        snprintf(
            text, sizeof(text),
            "Previous stage: %lu",
            (unsigned long)previous.stage
        );
        gfx_PrintStringXY(text, 16, 102);
    }

    gfx_PrintStringXY(
        write_ok
            ? "Write: OK"
            : "E101: write failed",
        16, 126
    );

    gfx_PrintStringXY(
        verify_ok
            ? "Read-back: OK"
            : "E102: read-back failed",
        16, 150
    );

    gfx_PrintStringXY("Any key: save exit and close", 16, 194);
    gfx_PrintStringXY("Reopen to check persistence.", 16, 212);

    gfx_BlitBuffer();

    while (os_GetCSC()) {
    }

    while (!os_GetCSC()) {
    }

    gfx_End();

    current.stage = 30;

    return write_record(&current) ? 0 : 1;
}
