#include <graphx.h>
#include <ti/getcsc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define LIMIT 1000L

typedef struct {
    int32_t p, q, f, r;
    bool imaginary, radical;
} root_t;

typedef struct {
    char lead[32], radicand[16], denominator[16];
    bool radical, fraction;
    int numerator_width, width;
} expression_t;

static int32_t coefficient_a;
static int32_t coefficient_b;
static int32_t coefficient_c;

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

    while (b) {
        remainder = a % b;
        a = b;
        b = remainder;
    }

    return a;
}

static void begin_screen(void)
{
    gfx_Begin();
    gfx_SetDrawBuffer();

    gfx_SetColor(0);
    gfx_SetTextFGColor(0);
    gfx_SetTextBGColor(255);
    gfx_SetTextScale(1, 1);
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
        case sk_0:
            return 0;

        case sk_1:
            return 1;

        case sk_2:
            return 2;

        case sk_3:
            return 3;

        case sk_4:
            return 4;

        case sk_5:
            return 5;

        case sk_6:
            return 6;

        case sk_7:
            return 7;

        case sk_8:
            return 8;

        case sk_9:
            return 9;

        default:
            return -1;
    }
}

static bool parse_number(
    const char *text,
    int32_t *value
)
{
    int32_t magnitude = 0;
    unsigned i = 0;
    bool negative = false;

    if (!text[0]) {
        *value = 0;
        return true;
    }

    if (text[0] == '-') {
        negative = true;
        i = 1;

        if (!text[i]) {
            return false;
        }
    }

    for (; text[i]; ++i) {
        if (text[i] < '0' || text[i] > '9') {
            return false;
        }

        magnitude =
            magnitude * 10L + (text[i] - '0');

        if (magnitude > LIMIT) {
            return false;
        }
    }

    *value = negative ? -magnitude : magnitude;

    return true;
}

static void draw_input(
    char fields[3][8],
    unsigned selected,
    bool replace,
    const char *message
)
{
    unsigned i;
    char text[64];

    page("QUADAPP / EXACT ROOTS");

    gfx_PrintStringXY(
        "A*x^2 + B*x + C = 0",
        12,
        42
    );

    gfx_PrintStringXY(
        "Integer coefficients: -1000 to 1000",
        12,
        59
    );

    for (i = 0; i < 3; ++i) {
        int y = 84 + (int)i * 30;

        const char *value =
            fields[i][0] ? fields[i] : "0";

        snprintf(
            text,
            sizeof(text),
            "%c = %s",
            (char)('A' + i),
            value
        );

        if (i == selected) {
            gfx_Rectangle(
                8,
                y - 7,
                304,
                25
            );

            gfx_PrintStringXY(
                ">",
                16,
                y
            );

            if (replace) {
                gfx_HorizLine(
                    60,
                    y + 11,
                    gfx_GetStringWidth(value)
                );
            }
        }

        gfx_PrintStringXY(
            text,
            28,
            y
        );
    }

    gfx_PrintStringXY(
        message,
        12,
        173
    );

    gfx_HorizLine(
        12,
        188,
        296
    );

    gfx_PrintStringXY(
        selected == 2
            ? "ENTER Solve     UP/DOWN Select"
            : "ENTER Next      UP/DOWN Select",
        12,
        199
    );

    gfx_PrintStringXY(
        "DEL Erase       CLEAR Clear field",
        12,
        212
    );

    gfx_PrintStringXY(
        "(-) Sign    GRAPH Solve    Y= Quit",
        12,
        225
    );

    gfx_BlitBuffer();
}

static bool input_coefficients(
    char fields[3][8],
    unsigned *selected
)
{
    const char *message =
        "Type to replace. Blank field = 0.";

    bool replace = true;

    for (;;) {
        uint8_t key;
        int digit;
        size_t length;
        char *field;

        draw_input(
            fields,
            *selected,
            replace,
            message
        );

        key = wait_key();

        message = replace
            ? "Type to replace. Blank field = 0."
            : "Editing: digits append to this field.";

        field = fields[*selected];
        length = strlen(field);
        digit = key_digit(key);

        if (key == sk_Yequ) {
            return false;
        }

        if (key == sk_Up || key == sk_Down) {
            *selected = (
                *selected +
                (key == sk_Up ? 2 : 1)
            ) % 3;

            replace = true;

            message =
                "Type to replace. Blank field = 0.";

            continue;
        }

        if (key == sk_Clear) {
            field[0] = '\0';
            replace = false;

            message =
                "Field cleared. Blank field = 0.";

            continue;
        }

        if (key == sk_Del) {
            if (length) {
                field[length - 1] = '\0';
            }

            replace = false;

            message =
                "Editing: digits append to this field.";

            continue;
        }

        if (key == sk_Chs || key == sk_Sub) {
            if (field[0] == '-') {
                memmove(
                    field,
                    field + 1,
                    length
                );
            } else if (length < 6) {
                memmove(
                    field + 1,
                    field,
                    length + 1
                );

                field[0] = '-';
            }

            replace = false;

            message =
                "Sign changed. Type digits to append.";

            continue;
        }

        if (digit >= 0) {
            int32_t value;

            if (replace) {
                field[0] = '\0';
                length = 0;
                replace = false;
            }

            if (length >= 6) {
                message =
                    "Maximum magnitude is 1000.";

                continue;
            }

            field[length] =
                (char)('0' + digit);

            field[length + 1] = '\0';

            if (!parse_number(field, &value)) {
                field[length] = '\0';

                message =
                    "Maximum magnitude is 1000.";
            } else {
                message =
                    "Editing: digits append to this field.";
            }

            continue;
        }

        if (key == sk_Enter && *selected < 2) {
            ++*selected;
            replace = true;

            message =
                "Type to replace. Blank field = 0.";

            continue;
        }

        if (key == sk_Enter || key == sk_Graph) {
            int32_t values[3];
            unsigned i;

            for (i = 0; i < 3; ++i) {
                if (!parse_number(
                    fields[i],
                    &values[i]
                )) {
                    break;
                }
            }

            if (i != 3) {
                *selected = i;
                replace = false;

                message =
                    "Enter digits after the minus sign.";

                continue;
            }

            coefficient_a = values[0];
            coefficient_b = values[1];
            coefficient_c = values[2];

            return true;
        }
    }
}

static root_t rational_root(
    int32_t p,
    int32_t q
)
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
    int32_t a = coefficient_a;
    int32_t b = coefficient_b;
    int32_t c = coefficient_c;

    int32_t d;
    int32_t magnitude;
    int32_t square_root = 0;
    int32_t factor = 1;
    int32_t remainder;
    int32_t i;
    int32_t p;
    int32_t q;
    int32_t divisor;

    memset(
        roots,
        0,
        sizeof(roots)
    );

    root_count = 0;
    all_numbers = false;
    repeated = false;

    if (a == 0) {
        if (b == 0) {
            all_numbers = c == 0;
        } else {
            roots[0] = rational_root(
                -c,
                b
            );

            root_count = 1;
        }

        return;
    }

    d = b * b - 4L * a * c;
    magnitude = abs32(d);

    while (
        (square_root + 1) *
        (square_root + 1) <= magnitude
    ) {
        ++square_root;
    }

    if (
        d >= 0 &&
        square_root * square_root == d
    ) {
        roots[0] = rational_root(
            -b + square_root,
            2L * a
        );

        root_count = 1;

        if (square_root == 0) {
            repeated = true;
        } else {
            roots[1] = rational_root(
                -b - square_root,
                2L * a
            );

            root_count = 2;
        }

        return;
    }

    remainder = magnitude;

    for (
        i = 2;
        i * i <= remainder;
        ++i
    ) {
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

    divisor = gcd32(
        gcd32(p, factor),
        q
    );

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

    const char *imaginary =
        root->imaginary ? "i" : "";

    int32_t factor = abs32(root->f);
    int denominator_width;

    memset(
        expression,
        0,
        sizeof(*expression)
    );

    expression->fraction =
        root->q != 1;

    expression->radical =
        root->radical && root->r != 1;

    snprintf(
        expression->denominator,
        sizeof(expression->denominator),
        "%ld",
        (long)root->q
    );

    if (!root->radical) {
        snprintf(
            expression->lead,
            sizeof(expression->lead),
            "%ld",
            (long)root->p
        );
    } else {
        if (root->p != 0) {
            snprintf(
                real_part,
                sizeof(real_part),
                "%ld",
                (long)root->p
            );

            sign =
                root->f < 0 ? "-" : "+";
        } else {
            sign =
                root->f < 0 ? "-" : "";
        }

        if (factor != 1) {
            snprintf(
                coefficient,
                sizeof(coefficient),
                "%ld",
                (long)factor
            );
        }

        snprintf(
            expression->lead,
            sizeof(expression->lead),
            "%s%s%s%s",
            real_part,
            sign,
            coefficient,
            imaginary
        );

        if (expression->radical) {
            snprintf(
                expression->radicand,
                sizeof(expression->radicand),
                "%ld",
                (long)root->r
            );
        }
    }

    expression->numerator_width =
        gfx_GetStringWidth(expression->lead);

    if (expression->radical) {
        expression->numerator_width +=
            17 +
            gfx_GetStringWidth(
                expression->radicand
            );
    }

    expression->width =
        expression->numerator_width;

    if (expression->fraction) {
        denominator_width =
            gfx_GetStringWidth(
                expression->denominator
            );

        if (
            denominator_width >
            expression->width
        ) {
            expression->width =
                denominator_width;
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
    int width =
        gfx_GetStringWidth(digits);

    gfx_PrintStringXY(
        digits,
        x + 12,
        y
    );

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
}

static void draw_expression(
    const expression_t *expression,
    int x,
    int middle
)
{
    int y = expression->fraction
        ? middle - 14
        : middle;

    int numerator_x = x + (
        expression->width -
        expression->numerator_width
    ) / 2;

    gfx_PrintStringXY(
        expression->lead,
        numerator_x,
        y
    );

    if (expression->radical) {
        draw_radical(
            numerator_x +
                gfx_GetStringWidth(
                    expression->lead
                ),
            y,
            expression->radicand
        );
    }

    if (expression->fraction) {
        int width = gfx_GetStringWidth(
            expression->denominator
        );

        gfx_HorizLine(
            x,
            middle + 1,
            expression->width
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
        prepare_expression(
            &roots[i],
            &expressions[i]
        );

        if (expressions[i].width > 296) {
            fits = false;
        }
    }

    for (;;) {
        page("QUADAPP / EXACT SOLUTIONS");

        snprintf(
            text,
            sizeof(text),
            "A=%ld  B=%ld  C=%ld",
            (long)coefficient_a,
            (long)coefficient_b,
            (long)coefficient_c
        );

        gfx_PrintStringXY(
            text,
            12,
            44
        );

        if (!fits) {
            gfx_PrintStringXY(
                "Answer exceeds display width.",
                12,
                88
            );
        } else if (!root_count) {
            gfx_PrintStringXY(
                all_numbers
                    ? "ALL COMPLEX NUMBERS"
                    : "NO SOLUTION",
                12,
                88
            );
        } else if (root_count == 1) {
            draw_expression(
                &expressions[0],
                12,
                96
            );

            if (repeated) {
                gfx_PrintStringXY(
                    "Repeated root",
                    12,
                    140
                );
            }
        } else {
            int combined =
                expressions[0].width +
                expressions[1].width +
                24;

            if (combined <= 296) {
                int x = 12;

                draw_expression(
                    &expressions[0],
                    x,
                    100
                );

                x +=
                    expressions[0].width + 6;

                gfx_PrintStringXY(
                    ",",
                    x,
                    100
                );

                draw_expression(
                    &expressions[1],
                    x + 18,
                    100
                );
            } else {
                draw_expression(
                    &expressions[0],
                    12,
                    86
                );

                if (
                    expressions[0].width <= 282
                ) {
                    gfx_PrintStringXY(
                        ",",
                        18 + expressions[0].width,
                        86
                    );
                } else {
                    gfx_PrintStringXY(
                        ",",
                        12,
                        116
                    );
                }

                draw_expression(
                    &expressions[1],
                    12,
                    150
                );
            }
        }

        gfx_HorizLine(
            12,
            201,
            296
        );

        gfx_PrintStringXY(
            "ENTER or CLEAR: edit coefficients",
            12,
            212
        );

        gfx_PrintStringXY(
            "Y=: quit",
            12,
            226
        );

        gfx_BlitBuffer();

        key = wait_key();

        if (
            key == sk_Enter ||
            key == sk_Clear
        ) {
            return true;
        }

        if (key == sk_Yequ) {
            return false;
        }
    }
}

int main(void)
{
    char fields[3][8] = {
        "",
        "",
        ""
    };

    unsigned selected = 0;
    bool running = true;

    begin_screen();

    while (running) {
        if (!input_coefficients(
            fields,
            &selected
        )) {
            break;
        }

        solve();

        running = show_results();
        selected = 0;
    }

    gfx_End();

    return 0;
}
