#include <graphx.h>
#include <keypadc.h>
#include <fileioc.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LOG_NAME "QFLOG"
#define LOG_MAGIC 0x51464C31UL

typedef struct {
    uint32_t magic;
    uint32_t launches;
    uint32_t events;
    uint32_t last_stage;
} log_record_t;

static log_record_t record;
static bool log_ok;
static bool old_log_invalid;
static bool previous_log_found;

static bool save_log(void)
{
    uint8_t handle = ti_Open(LOG_NAME, "w");
    bool written;
    int closed;

    if (!handle) {
        return false;
    }

    written = ti_Write(
        &record, sizeof(record), 1, handle
    ) == 1;

    closed = ti_Close(handle);

    return written && closed != 0;
}

static void load_log(void)
{
    uint8_t handle = ti_Open(LOG_NAME, "r");

    memset(&record, 0, sizeof(record));
    record.magic = LOG_MAGIC;

    if (handle) {
        log_record_t previous;
        size_t count = ti_Read(
            &previous, sizeof(previous), 1, handle
        );

        ti_Close(handle);

        if (count == 1 && previous.magic == LOG_MAGIC) {
            record = previous;
            previous_log_found = true;
        } else {
            old_log_invalid = true;
        }
    }
}

static void draw_screen(void)
{
    char text[64];

    gfx_FillScreen(255);
    gfx_SetColor(0);
    gfx_SetTextFGColor(0);
    gfx_SetTextBGColor(255);
    gfx_SetTextScale(1, 1);

    gfx_PrintStringXY("QUADRATIC / NATIVE TEST", 16, 16);
    gfx_HorizLine(16, 34, 288);

    gfx_PrintStringXY("Build 0.1: interface + logging", 16, 48);
    gfx_PrintStringXY("This is not the solver yet.", 16, 64);

    snprintf(
        text, sizeof(text),
        "Launch count: %lu",
        (unsigned long)record.launches
    );
    gfx_PrintStringXY(text, 16, 92);

    snprintf(
        text, sizeof(text),
        "Test events: %lu",
        (unsigned long)record.events
    );
    gfx_PrintStringXY(text, 16, 108);

    snprintf(
        text, sizeof(text),
        "Last stage: %lu",
        (unsigned long)record.last_stage
    );
    gfx_PrintStringXY(text, 16, 124);

    gfx_PrintStringXY(
        log_ok ? "Log write: OK" : "E101: LOG WRITE FAILED",
        16, 144
    );

    if (old_log_invalid) {
        gfx_PrintStringXY("Old log invalid: replaced", 16, 160);
    } else if (previous_log_found) {
        gfx_PrintStringXY("Previous log loaded", 16, 160);
    } else {
        gfx_PrintStringXY("New log created", 16, 160);
    }

    gfx_HorizLine(16, 184, 288);
    gfx_PrintStringXY("ENTER: save test event", 16, 198);
    gfx_PrintStringXY("CLEAR: save and exit", 16, 214);

    gfx_BlitBuffer();
}

int main(void)
{
    bool enter_was_down = false;

    load_log();

    record.launches++;
    record.last_stage = 10;
    log_ok = save_log();

    gfx_Begin();
    gfx_SetDrawBuffer();

    draw_screen();

    do {
        kb_Scan();
    } while (kb_AnyKey());

    for (;;) {
        bool enter_down;

        kb_Scan();

        if (kb_Data[6] & kb_Clear) {
            break;
        }

        enter_down = (kb_Data[6] & kb_Enter) != 0;

        if (enter_down && !enter_was_down) {
            record.events++;
            record.last_stage = 20;
            log_ok = save_log();
            draw_screen();
        }

        enter_was_down = enter_down;
    }

    gfx_End();

    record.last_stage = 30;

    if (!save_log()) {
        return 1;
    }

    return 0;
}
