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
        record,
        sizeof(*record),
        1,
        handle
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
        record,
        sizeof(*record),
        1,
        handle
    ) == 1;

    closed = ti_Close(handle);

    return read_ok &&
           closed != 0 &&
           record->magic == LOG_MAGIC;
}

static int draw_radical(int x, int y, const char *digits)
{
    int width = gfx_GetStringWidth(digits);

    gfx_PrintStringXY(digits, x + 12, y);

    gfx_Line(
        x,
        y + 5,
        x + 3,
        y + 8
    );

    gfx_Line(
        x + 3,
        y + 8,
        x + 8,
        y - 3
    );

    gfx_HorizLine(
        x + 8,
        y - 3,
        width + 7
    );

    return x + width + 17;
}

static int draw_fraction_root(
    int x,
    int y,
    bool positive
)
{
    const char *prefix = positive ? "-3+" : "-3-";

    int prefix_width = gfx_GetStringWidth(prefix);
    int radical_width = gfx_GetStringWidth("17") + 17;
    int numerator_width = prefix_width + radical_width;
    int fraction_width = numerator_width + 8;
    int denominator_width = gfx_GetStringWidth("2");

    gfx_PrintStringXY(
        prefix,
        x + 4,
        y
    );

    draw_radical(
        x + 4 + prefix_width,
        y,
        "17"
    );

    gfx_HorizLine(
        x,
        y + 14,
        fraction_width
    );

    gfx_PrintStringXY(
        "2",
        x + (fraction_width - denominator_width) / 2,
        y + 20
    );

    return x + fraction_width;
}

static int draw_complex_root(
    int x,
    int y,
    bool positive
)
{
    const char *prefix = positive ? "1+i" : "1-i";

    gfx_PrintStringXY(prefix, x, y);

    x += gfx_GetStringWidth(prefix);

    return draw_radical(x, y, "7");
}

static int draw_imaginary_root(
    int x,
    int y,
    bool positive
)
{
    const char *prefix = positive ? "i" : "-i";

    gfx_PrintStringXY(prefix, x, y);

    x += gfx_GetStringWidth(prefix);

    return draw_radical(x, y, "7");
}

static void draw_examples(void)
{
    int x;

    gfx_PrintStringXY(
        "QFORM: EXACT ANSWER DISPLAY",
        12,
        12
    );

    gfx_HorizLine(12, 28, 296);

    gfx_PrintStringXY(
        "x^2 + 3x - 2 = 0",
        12,
        42
    );

    x = draw_fraction_root(12, 64, true);

    gfx_PrintStringXY(
        ",",
        x + 6,
        75
    );

    draw_fraction_root(
        x + 22,
        64,
        false
    );

    gfx_PrintStringXY(
        "x^2 - 2x + 8 = 0",
        12,
        112
    );

    x = draw_complex_root(12, 134, true);

    gfx_PrintStringXY(
        ",",
        x + 4,
        134
    );

    draw_complex_root(
        x + 20,
        134,
        false
    );

    gfx_PrintStringXY(
        "x^2 + 7 = 0",
        12,
        166
    );

    x = draw_imaginary_root(12, 188, true);

    gfx_PrintStringXY(
        ",",
        x + 4,
        188
    );

    draw_imaginary_root(
        x + 20,
        188,
        false
    );
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

    draw_examples();

    snprintf(
        text,
        sizeof(text),
        "Log:%s  Run:%lu  Any key exits",
        (write_ok && verify_ok) ? "OK" : "FAIL",
        (unsigned long)current.launches
    );

    gfx_PrintStringXY(
        text,
        12,
        222
    );

    gfx_BlitBuffer();

    while (os_GetCSC()) {
    }

    while (!os_GetCSC()) {
    }

    gfx_End();

    current.stage = 30;

    return write_record(&current) ? 0 : 1;
}
