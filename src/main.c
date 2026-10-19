#include <graphx.h>
#include <fileioc.h>
#include <ti/getcsc.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define LOG_NAME "QAPPLOG"
#define LOG_MAGIC 0x51415032UL
#define LIMIT 1000L

typedef struct {
    uint32_t magic;
    uint32_t launches;
    uint32_t calculations;
    int32_t a;
    int32_t b;
    int32_t c;
    int32_t discriminant;
    uint32_t stage;
    uint32_t error;
    uint32_t clean_exit;
} log_record_t;

typedef struct {
    int32_t p;
    int32_t q;
    int32_t f;
    int32_t r;
    bool imaginary;
    bool radical;
} root_t;

typedef struct {
    char lead[32];
    char radicand[16];
    char denominator[16];
    bool radical;
    bool fraction;
    int numerator_width;
    int width;
} expression_t;

static log_record_t log_data;
static bool log_ok = true;

static root_t roots[2];
static unsigned root_count;
static bool all_numbers;
static bool repeated;

static int32_t abs32(int32_t value)
{
    return value < 0 ? -value : value;
}

static int32_t gcd32(int32_t a, int32_t b)
{
    int32_t remainder;

    a = abs32(a);
    b = abs32(b);

    while (b != 0) {
        remainder = a % b;
        a = b;
        b = remainder;
    }

    return a;
}

static bool read_log(log_record_t *record)
{
    uint8_t handle = ti_Open(LOG_NAME, "r");
    bool ok;
    int closed;

    if (!handle) {
        return false;
    }

    ok = ti_Read(record, sizeof(*record), 1, handle) == 1;
    closed = ti_Close(handle);

    return ok && closed != 0 &&
           record->magic == LOG_MAGIC;
}

static bool save_log(void)
{
    log_record_t check = {0};
    uint8_t handle = ti_Open(LOG_NAME, "w");
    bool ok;
    int closed;

    if (!handle) {
        log_ok = false;
        return false;
    }

    ok = ti_Write(
        &log_data, sizeof(log_data), 1, handle
    ) == 1;

    closed = ti_Close(handle);

    if (!ok || closed == 0 || !read_log(&check)) {
        log_ok = false;
        return false;
    }

    ok = check.magic == log_data.magic &&
         check.launches == log_data.launches &&
         check.calculations == log_data.calculations &&
         check.a == log_data.a &&
         check.b == log_data.b &&
         check.c == log_data.c &&
         check.discriminant == log_data.discriminant &&
         check.stage == log_data.stage &&
         check.error == log_data.error &&
         check.clean_exit == log_data.clean_exit;

    if (!ok) {
        log_ok = false;
    }

    return ok;
}

static void begin_screen(void)
{
    gfx_Begin();
    gfx_SetDrawBuffer();
    gfx_SetColor(0);
    gfx_SetTextFGColor(0);
    gfx_SetTextBGColor(255);
    gfx_SetTextScale(1, 1);
    gfx_SetTextConfig(gfx_text_clip);
}

static void page(const char *title)
{
    gfx_FillScreen(255);
    gfx_SetColor(0);
    gfx_PrintStringXY(title, 12, 12);
    gfx_HorizLine(12, 28, 296);
}

static uint8_t wait_key(void)
{
    uint8_t key;

    do {
        key = os_GetCSC();
    } while (!key);

    return key;
}

static int key_digit(uint8_t key)
{
    switch (key) {
        case sk_0: return 0;
        case sk_1: return 1;
        case sk_2: return 2;
        case sk_3: return 3;
        case sk_4: return 4;
        case sk_5: return 5;
        case sk_6: return 6;
        case sk_7: return 7;
        case sk_8: return 8;
        case sk_9: return 9;
        default: return -1;
    }
}

static bool parse_number(const char *text, int32_t *value)
{
    int32_t magnitude = 0;
    unsigned i = 0;
    bool negative = false;

    if (text[0] == '\0') {
        *value = 0;
        return true;
    }

    if (text[0] == '-') {
        negative = true;
        i = 1;

        if (text[i] == '\0') {
            return false;
        }
    }

    for (; text[i] != '\0'; ++i) {
        if (text[i] < '0' || text[i] > '9') {
            return false;
        }

        magnitude = magnitude * 10L + (text[i] - '0');

        if (magnitude > LIMIT) {
            return false;
        }
    }

    *value = negative ? -magnitude : magnitude;
    return true;
}

static void diagnostics(void)
{
    char text[64];
    uint8_t key;

    page("QUADAPP / DIAGNOSTICS");

    snprintf(text, sizeof(text), "Launches: %lu",
             (unsigned long)log_data.launches);
    gfx_PrintStringXY(text, 12, 44);

    snprintf(text, sizeof(text), "Calculations: %lu",
             (unsigned long)log_data.calculations);
    gfx_PrintStringXY(text, 12, 64);

    snprintf(text, sizeof(text), "A=%ld B=%ld C=%ld",
             (long)log_data.a,
             (long)log_data.b,
             (long)log_data.c);
    gfx_PrintStringXY(text, 12, 84);

    snprintf(text, sizeof(text), "Discriminant: %ld",
             (long)log_data.discriminant);
    gfx_PrintStringXY(text, 12, 104);

    snprintf(text, sizeof(text), "Stage: %lu  Error: %lu",
             (unsigned long)log_data.stage,
             (unsigned long)log_data.error);
    gfx_PrintStringXY(text, 12, 124);

    gfx_PrintStringXY(
        log_ok ? "Log write/read-back: OK"
               : "E101: LOG STORAGE FAILURE",
        12, 148
    );

    gfx_PrintStringXY("20=input, 30=solved, 40=layout", 12, 172);
    gfx_PrintStringXY("Error 201: invalid coefficient", 12, 190);
    gfx_PrintStringXY("ENTER or CLEAR: return", 12, 220);
    gfx_BlitBuffer();

    do {
        key = wait_key();
    } while (key != sk_Enter && key != sk_Clear);
}

static void draw_input(
    char fields[3][8],
    unsigned selected,
    const char *message
)
{
    unsigned i;
    char text[64];

    page("QUADAPP / EXACT ROOTS");
    gfx_PrintStringXY("A*x^2 + B*x + C = 0", 12, 44);
    gfx_PrintStringXY("Integers: -1000 through 1000", 12, 62);

    for (i = 0; i < 3; ++i) {
        int y = 88 + (int)i * 30;

        snprintf(
            text, sizeof(text),
            "%c = %s",
            (char)('A' + i),
            fields[i][0] ? fields[i] : "0"
        );

        if (i == selected) {
            gfx_Rectangle(8, y - 6, 304, 24);
        }

        gfx_PrintStringXY(text, 18, y);
    }

    gfx_PrintStringXY(message, 12, 179);
    gfx_PrintStringXY("ENTER: next/solve  GRAPH: solve", 12, 198);
    gfx_PrintStringXY("DEL: erase  +/-: sign  UP/DN: field", 12, 212);
    gfx_PrintStringXY("MODE: logs  CLEAR: exit", 12, 226);
    gfx_BlitBuffer();
}

static bool input_coefficients(
    char fields[3][8],
    unsigned *selected
)
{
    const char *message = "Blank fields mean zero.";

    for (;;) {
        uint8_t key;
        int digit;
        size_t length;
        char *field;

        draw_input(fields, *selected, message);
        key = wait_key();
        message = "Blank fields mean zero.";
        field = fields[*selected];
        length = strlen(field);
        digit = key_digit(key);

        if (key == sk_Clear) {
            return false;
        }

        if (key == sk_Mode) {
            diagnostics();
            continue;
        }

        if (key == sk_Up) {
            *selected = (*selected + 2) % 3;
            continue;
        }

        if (key == sk_Down) {
            *selected = (*selected + 1) % 3;
            continue;
        }

        if (key == sk_Del) {
            if (length) {
                field[length - 1] = '\0';
            }
            continue;
        }

        if (key == sk_Chs || key == sk_Sub) {
            if (field[0] == '-') {
                memmove(field, field + 1, length);
            } else if (length < 6) {
                memmove(field + 1, field, length + 1);
                field[0] = '-';
            }
            continue;
        }

        if (digit >= 0) {
            int32_t value;

            if (length >= 6) {
                message = "Maximum magnitude is 1000.";
                continue;
            }

            field[length] = (char)('0' + digit);
            field[length + 1] = '\0';

            if (!parse_number(field, &value)) {
                field[length] = '\0';
                message = "Maximum magnitude is 1000.";
            }

            continue;
        }

        if (key == sk_Enter && *selected < 2) {
            ++*selected;
            continue;
        }

        if (key == sk_Enter || key == sk_Graph) {
            int32_t values[3];
            unsigned i;

            for (i = 0; i < 3; ++i) {
                if (!parse_number(fields[i], &values[i])) {
                    break;
                }
            }

            if (i != 3) {
                *selected = i;
                message = "Enter digits after the minus sign.";
                log_data.error = 201;
                continue;
            }

            log_data.a = values[0];
            log_data.b = values[1];
            log_data.c = values[2];
            log_data.discriminant = 0;
            log_data.error = 0;
            return true;
        }
    }
}

static root_t rational_root(int32_t p, int32_t q)
{
    root_t result = {0};
    int32_t divisor;

    if (q < 0) {
        p = -p;
        q = -q;
    }

    divisor = gcd32(p, q);

    result.p = p / divisor;
    result.q = q / divisor;

    return result;
}

static void solve(void)
{
    int32_t a = log_data.a;
    int32_t b = log_data.b;
    int32_t c = log_data.c;
    int32_t d;
    int32_t magnitude;
    int32_t square_root = 0;
    int32_t factor = 1;
    int32_t remainder;
    int32_t i;
    int32_t p;
    int32_t q;
    int32_t divisor;

    memset(roots, 0, sizeof(roots));
    root_count = 0;
    all_numbers = false;
    repeated = false;

    if (a == 0) {
        if (b == 0) {
            all_numbers = c == 0;
        } else {
            roots[0] = rational_root(-c, b);
            root_count = 1;
        }
        return;
    }

    d = b * b - 4L * a * c;
    log_data.discriminant = d;
    magnitude = abs32(d);

    while ((square_root + 1) * (square_root + 1)
           <= magnitude) {
        ++square_root;
    }

    if (d >= 0 && square_root * square_root == d) {
        roots[0] = rational_root(-b + square_root, 2L * a);
        root_count = 1;

        if (square_root == 0) {
            repeated = true;
        } else {
            roots[1] = rational_root(-b - square_root, 2L * a);
            root_count = 2;
        }
        return;
    }

    remainder = magnitude;

    for (i = 2; i * i <= remainder; ++i) {
        while (remainder % (i * i) == 0) {
            remainder /= i * i;
            factor *= i;
        }
    }

    p = -b;
    q = 2L * a;

    if (q < 0) {
        p = -p;
        q = -q;
    }

    divisor = gcd32(gcd32(p, factor), q);

    roots[0].p = p / divisor;
    roots[0].q = q / divisor;
    roots[0].f = factor / divisor;
    roots[0].r = remainder;
    roots[0].imaginary = d < 0;
    roots[0].radical = true;

    roots[1] = roots[0];
    roots[1].f = -roots[1].f;

    root_count = 2;
}

static void prepare_expression(
    const root_t *root,
    expression_t *expression
)
{
    char coefficient[16] = "";
    char real_part[16] = "";
    const char *sign = "";
    const char *imaginary = root->imaginary ? "i" : "";
    int32_t factor = abs32(root->f);
    int denominator_width;

    memset(expression, 0, sizeof(*expression));

    expression->fraction = root->q != 1;
    expression->radical = root->radical && root->r != 1;

    snprintf(
        expression->denominator,
        sizeof(expression->denominator),
        "%ld", (long)root->q
    );

    if (!root->radical) {
        snprintf(
            expression->lead,
            sizeof(expression->lead),
            "%ld", (long)root->p
        );
    } else {
        if (root->p != 0) {
            snprintf(
                real_part, sizeof(real_part),
                "%ld", (long)root->p
            );
            sign = root->f < 0 ? "-" : "+";
        } else {
            sign = root->f < 0 ? "-" : "";
        }

        if (factor != 1) {
            snprintf(
                coefficient, sizeof(coefficient),
                "%ld", (long)factor
            );
        }

        snprintf(
            expression->lead,
            sizeof(expression->lead),
            "%s%s%s%s",
            real_part, sign, coefficient, imaginary
        );

        if (expression->radical) {
            snprintf(
                expression->radicand,
                sizeof(expression->radicand),
                "%ld", (long)root->r
            );
        }
    }

    expression->numerator_width =
        gfx_GetStringWidth(expression->lead);

    if (expression->radical) {
        expression->numerator_width +=
            17 + gfx_GetStringWidth(expression->radicand);
    }

    expression->width = expression->numerator_width;

    if (expression->fraction) {
        denominator_width =
            gfx_GetStringWidth(expression->denominator);

        if (denominator_width > expression->width) {
            expression->width = denominator_width;
        }

        expression->width += 8;
    }
}

static void draw_radical(
    int x,
    int y,
    const char *digits
)
{
    int width = gfx_GetStringWidth(digits);

    gfx_PrintStringXY(digits, x + 12, y);
    gfx_Line(x, y + 5, x + 3, y + 8);
    gfx_Line(x + 3, y + 8, x + 8, y - 3);
    gfx_HorizLine(x + 8, y - 3, width + 7);
}

static void draw_expression(
    const expression_t *expression,
    int x,
    int middle
)
{
    int y = expression->fraction ? middle - 14 : middle;
    int numerator_x =
        x + (expression->width -
             expression->numerator_width) / 2;

    gfx_PrintStringXY(expression->lead, numerator_x, y);

    if (expression->radical) {
        draw_radical(
            numerator_x +
                gfx_GetStringWidth(expression->lead),
            y,
            expression->radicand
        );
    }

    if (expression->fraction) {
        int width =
            gfx_GetStringWidth(expression->denominator);

        gfx_HorizLine(
            x, middle + 1, expression->width
        );

        gfx_PrintStringXY(
            expression->denominator,
            x + (expression->width - width) / 2,
            middle + 10
        );
    }
}

static bool show_results(void)
{
    expression_t expressions[2];
    char text[64];
    uint8_t key;
    unsigned i;
    bool fits = true;

    for (i = 0; i < root_count; ++i) {
        prepare_expression(&roots[i], &expressions[i]);

        if (expressions[i].width > 296) {
            fits = false;
        }
    }

    for (;;) {
        page("QUADAPP / EXACT SOLUTIONS");

        snprintf(
            text, sizeof(text),
            "A=%ld  B=%ld  C=%ld",
            (long)log_data.a,
            (long)log_data.b,
            (long)log_data.c
        );
        gfx_PrintStringXY(text, 12, 44);

        if (!fits) {
            gfx_PrintStringXY("E301: answer exceeds display width", 12, 88);
        } else if (root_count == 0) {
            gfx_PrintStringXY(
                all_numbers ? "ALL COMPLEX NUMBERS"
                            : "NO SOLUTION",
                12, 88
            );
        } else if (root_count == 1) {
            draw_expression(&expressions[0], 12, 96);

            if (repeated) {
                gfx_PrintStringXY("Repeated root", 12, 140);
            }
        } else {
            int combined =
                expressions[0].width +
                expressions[1].width + 24;

            if (combined <= 296) {
                int x = 12;

                draw_expression(&expressions[0], x, 100);
                x += expressions[0].width + 6;
                gfx_PrintStringXY(",", x, 100);
                draw_expression(&expressions[1], x + 18, 100);
            } else {
                draw_expression(&expressions[0], 12, 86);
                gfx_PrintStringXY(
                    ",",
                    18 + expressions[0].width,
                    86
                );
                draw_expression(&expressions[1], 12, 150);
            }
        }

        gfx_PrintStringXY(
            log_ok ? "Diagnostic storage: OK"
                   : "E101: diagnostic storage failed",
            12, 190
        );

        gfx_PrintStringXY("ENTER: edit coefficients", 12, 208);
        gfx_PrintStringXY("MODE: logs  CLEAR: exit", 12, 224);
        gfx_BlitBuffer();

        key = wait_key();

        if (key == sk_Mode) {
            diagnostics();
        } else if (key == sk_Enter) {
            return true;
        } else if (key == sk_Clear) {
            return false;
        }
    }
}

int main(void)
{
    char fields[3][8] = {"", "", ""};
    unsigned selected = 0;
    bool running = true;

    if (!read_log(&log_data)) {
        memset(&log_data, 0, sizeof(log_data));
        log_data.magic = LOG_MAGIC;
    }

    ++log_data.launches;
    log_data.clean_exit = 0;
    save_log();

    while (running) {
        begin_screen();

        if (!input_coefficients(fields, &selected)) {
            gfx_End();
            break;
        }

        gfx_End();

        ++log_data.calculations;
        log_data.stage = 20;
        save_log();

        solve();

        log_data.stage = 30;
        save_log();

        begin_screen();

        /*
         * Prepare/check layout before recording that the
         * result is ready. No AppVar writes occur while
         * graphics mode is active.
         */
        {
            unsigned i;

            for (i = 0; i < root_count; ++i) {
                expression_t expression;

                prepare_expression(&roots[i], &expression);

                if (expression.width > 296) {
                    log_data.error = 301;
                }
            }
        }

        gfx_End();

        log_data.stage = 40;
        save_log();

        begin_screen();
        running = show_results();
        gfx_End();
    }

    log_data.clean_exit = 1;
    save_log();

    return log_ok ? 0 : 1;
}
