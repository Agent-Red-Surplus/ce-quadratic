#include <graphx.h>
#include <ti/getcsc.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define APP_VERSION "0.4.0"
#define LIMIT 1000L
#define WORK_MAX 360
#define WORK_COLS 80
#define WORK_PAGE 10

typedef struct {
    int32_t p, q, f, r;
    bool imaginary, radical;
} root_t;

typedef struct {
    char lead[32], radicand[16], denominator[16];
    bool radical, fraction;
    int numerator_width, width;
} expression_t;

static int32_t coefficient_a, coefficient_b, coefficient_c;
static root_t roots[2];
static unsigned root_count;
static bool all_numbers, repeated;
static char coefficient_fields[3][8] = {"", "", ""};
static char work_lines[WORK_MAX][WORK_COLS];
static unsigned work_count;
static bool work_overflow;

static int32_t abs32(int32_t v) { return v < 0 ? -v : v; }

static int32_t gcd32(int32_t a, int32_t b)
{
    int32_t t;
    a = abs32(a);
    b = abs32(b);
    while (b) { t = a % b; a = b; b = t; }
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
    do { key = os_GetCSC(); } while (!key);
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
    if (!text[0]) { *value = 0; return true; }
    if (text[0] == '-') {
        negative = true;
        i = 1;
        if (!text[i]) return false;
    }
    for (; text[i]; ++i) {
        if (text[i] < '0' || text[i] > '9') return false;
        magnitude = magnitude * 10L + (text[i] - '0');
        if (magnitude > LIMIT) return false;
    }
    *value = negative ? -magnitude : magnitude;
    return true;
}

static void draw_input(unsigned selected, bool replace, const char *message)
{
    unsigned i;
    char text[64];
    page("ALGEBRA / QUADRATIC + LINEAR");
    gfx_PrintStringXY("A*x^2 + B*x + C = 0", 12, 42);
    gfx_PrintStringXY("Integer coefficients: -1000 to 1000", 12, 59);
    for (i = 0; i < 3; ++i) {
        int y = 84 + (int)i * 30;
        const char *value = coefficient_fields[i][0]
            ? coefficient_fields[i] : "0";
        snprintf(text, sizeof(text), "%c = %s", (char)('A' + i), value);
        if (i == selected) {
            gfx_Rectangle(8, y - 7, 304, 25);
            gfx_PrintStringXY(">", 16, y);
            if (replace) gfx_HorizLine(60, y + 11, gfx_GetStringWidth(value));
        }
        gfx_PrintStringXY(text, 28, y);
    }
    gfx_PrintStringXY(message, 12, 173);
    gfx_HorizLine(12, 188, 296);
    gfx_PrintStringXY(selected == 2
        ? "ENTER Solve     UP/DOWN Select"
        : "ENTER Next      UP/DOWN Select", 12, 199);
    gfx_PrintStringXY("DEL Erase       CLEAR Clear field", 12, 212);
    gfx_PrintStringXY("(-) Sign    GRAPH Solve    Y= Menu", 12, 225);
    gfx_BlitBuffer();
}

static bool input_coefficients(unsigned *selected)
{
    const char *message = "Type to replace. Blank field = 0.";
    bool replace = true;
    for (;;) {
        uint8_t key;
        int digit;
        size_t length;
        char *field;
        draw_input(*selected, replace, message);
        key = wait_key();
        message = replace ? "Type to replace. Blank field = 0."
                          : "Editing: digits append to this field.";
        field = coefficient_fields[*selected];
        length = strlen(field);
        digit = key_digit(key);
        if (key == sk_Yequ) return false;
        if (key == sk_Up || key == sk_Down) {
            *selected = (*selected + (key == sk_Up ? 2 : 1)) % 3;
            replace = true;
            message = "Type to replace. Blank field = 0.";
            continue;
        }
        if (key == sk_Clear) {
            field[0] = '\0';
            replace = false;
            message = "Field cleared. Blank field = 0.";
            continue;
        }
        if (key == sk_Del) {
            if (length) field[length - 1] = '\0';
            replace = false;
            message = "Editing: digits append to this field.";
            continue;
        }
        if (key == sk_Chs || key == sk_Sub) {
            if (field[0] == '-') memmove(field, field + 1, length);
            else if (length < 6) {
                memmove(field + 1, field, length + 1);
                field[0] = '-';
            }
            replace = false;
            message = "Sign changed. Type digits to append.";
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
                message = "Maximum magnitude is 1000.";
                continue;
            }
            field[length] = (char)('0' + digit);
            field[length + 1] = '\0';
            if (!parse_number(field, &value)) {
                field[length] = '\0';
                message = "Maximum magnitude is 1000.";
            } else message = "Editing: digits append to this field.";
            continue;
        }
        if (key == sk_Enter && *selected < 2) {
            ++*selected;
            replace = true;
            message = "Type to replace. Blank field = 0.";
            continue;
        }
        if (key == sk_Enter || key == sk_Graph) {
            int32_t values[3];
            unsigned i;
            for (i = 0; i < 3; ++i)
                if (!parse_number(coefficient_fields[i], &values[i])) break;
            if (i != 3) {
                *selected = i;
                replace = false;
                message = "Enter digits after the minus sign.";
                continue;
            }
            coefficient_a = values[0];
            coefficient_b = values[1];
            coefficient_c = values[2];
            return true;
        }
    }
}

static root_t rational_root(int32_t p, int32_t q)
{
    root_t result = {0};
    int32_t divisor;
    if (q < 0) { p = -p; q = -q; }
    divisor = gcd32(p, q);
    result.p = p / divisor;
    result.q = q / divisor;
    return result;
}

static void solve(void)
{
    int32_t a = coefficient_a, b = coefficient_b, c = coefficient_c;
    int32_t d, magnitude, square_root = 0, factor = 1;
    int32_t remainder, i, p, q, divisor;
    memset(roots, 0, sizeof(roots));
    root_count = 0;
    all_numbers = false;
    repeated = false;
    if (a == 0) {
        if (b == 0) all_numbers = c == 0;
        else { roots[0] = rational_root(-c, b); root_count = 1; }
        return;
    }
    d = b * b - 4L * a * c;
    magnitude = abs32(d);
    while ((square_root + 1) * (square_root + 1) <= magnitude)
        ++square_root;
    if (d >= 0 && square_root * square_root == d) {
        roots[0] = rational_root(-b + square_root, 2L * a);
        root_count = 1;
        if (square_root == 0) repeated = true;
        else {
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
    if (q < 0) { p = -p; q = -q; }
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

static void prepare_expression(const root_t *root, expression_t *e)
{
    char coefficient[16] = "", real_part[16] = "";
    const char *sign = "";
    const char *imaginary = root->imaginary ? "i" : "";
    int32_t factor = abs32(root->f);
    int denominator_width;
    memset(e, 0, sizeof(*e));
    e->fraction = root->q != 1;
    e->radical = root->radical && root->r != 1;
    snprintf(e->denominator, sizeof(e->denominator), "%ld", (long)root->q);
    if (!root->radical) {
        snprintf(e->lead, sizeof(e->lead), "%ld", (long)root->p);
    } else {
        if (root->p != 0) {
            snprintf(real_part, sizeof(real_part), "%ld", (long)root->p);
            sign = root->f < 0 ? "-" : "+";
        } else sign = root->f < 0 ? "-" : "";
        if (factor != 1)
            snprintf(coefficient, sizeof(coefficient), "%ld", (long)factor);
        snprintf(e->lead, sizeof(e->lead), "%s%s%s%s",
                 real_part, sign, coefficient, imaginary);
        if (e->radical)
            snprintf(e->radicand, sizeof(e->radicand), "%ld", (long)root->r);
    }
    e->numerator_width = gfx_GetStringWidth(e->lead);
    if (e->radical) e->numerator_width += 17 + gfx_GetStringWidth(e->radicand);
    e->width = e->numerator_width;
    if (e->fraction) {
        denominator_width = gfx_GetStringWidth(e->denominator);
        if (denominator_width > e->width) e->width = denominator_width;
        e->width += 8;
    }
}

static void draw_radical(int x, int y, const char *digits)
{
    int width = gfx_GetStringWidth(digits);
    gfx_PrintStringXY(digits, x + 12, y);
    gfx_Line(x, y + 5, x + 3, y + 8);
    gfx_Line(x + 3, y + 8, x + 8, y - 3);
    gfx_HorizLine(x + 8, y - 3, width + 7);
}

static void draw_expression(const expression_t *e, int x, int middle)
{
    int y = e->fraction ? middle - 14 : middle;
    int nx = x + (e->width - e->numerator_width) / 2;
    gfx_PrintStringXY(e->lead, nx, y);
    if (e->radical)
        draw_radical(nx + gfx_GetStringWidth(e->lead), y, e->radicand);
    if (e->fraction) {
        int width = gfx_GetStringWidth(e->denominator);
        gfx_HorizLine(x, middle + 1, e->width);
        gfx_PrintStringXY(e->denominator, x + (e->width - width) / 2,
                         middle + 10);
    }
}

static void fraction_text(int32_t p, int32_t q, char *out, size_t size)
{
    root_t value = rational_root(p, q);
    if (value.q == 1) snprintf(out, size, "%ld", (long)value.p);
    else snprintf(out, size, "%ld/%ld", (long)value.p, (long)value.q);
}

static void root_text(const root_t *root, char *out, size_t size)
{
    expression_t e;
    char numerator[80];
    prepare_expression(root, &e);
    snprintf(numerator, sizeof(numerator), "%s", e.lead);
    if (e.radical) {
        size_t used = strlen(numerator);
        const char *join = "";
        if (used && numerator[used - 1] >= '0' && numerator[used - 1] <= '9')
            join = "*";
        snprintf(numerator + used, sizeof(numerator) - used,
                 "%ssqrt(%s)", join, e.radicand);
    }
    if (e.fraction && root->radical)
        snprintf(out, size, "(%s)/%s", numerator, e.denominator);
    else if (e.fraction)
        snprintf(out, size, "%s/%s", numerator, e.denominator);
    else snprintf(out, size, "%s", numerator);
}

static void store_work(const char *text, size_t length)
{
    if (work_count >= WORK_MAX || length >= WORK_COLS) {
        work_overflow = true;
        return;
    }
    memcpy(work_lines[work_count], text, length);
    work_lines[work_count][length] = '\0';
    ++work_count;
}

/* Wrap by measured pixel width rather than assuming fixed-width text. */
static void add_work(const char *format, ...)
{
    char text[256];
    va_list args;
    const char *p;
    int result;
    va_start(args, format);
    result = vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    if (result < 0 || (size_t)result >= sizeof(text)) work_overflow = true;
    p = text;
    if (!*p) { store_work("", 0); return; }
    while (*p) {
        char candidate[WORK_COLS];
        size_t n = 0, last_space = 0, take;
        while (*p == ' ') ++p;
        if (!*p) break;
        while (p[n] && n < WORK_COLS - 1) {
            candidate[n] = p[n];
            candidate[n + 1] = '\0';
            if (gfx_GetStringWidth(candidate) > 292) break;
            if (p[n] == ' ') last_space = n;
            ++n;
        }
        if (!n) { work_overflow = true; return; }
        take = n;
        if (p[n] && last_space) take = last_space;
        store_work(p, take);
        p += take;
    }
}

static void explain_fraction(const char *name, int32_t p, int32_t q)
{
    int32_t g;
    char answer[40];
    add_work("%s = (%ld)/(%ld)", name, (long)p, (long)q);
    if (q < 0) {
        p = -p;
        q = -q;
        add_work("Multiply top and bottom by -1:");
        add_work("%s = %ld/%ld", name, (long)p, (long)q);
    }
    g = gcd32(p, q);
    if (g > 1) {
        add_work("The greatest common factor is %ld.", (long)g);
        add_work("Divide the numerator: %ld / %ld = %ld.",
                 (long)p, (long)g, (long)(p/g));
        add_work("Divide the denominator: %ld / %ld = %ld.",
                 (long)q, (long)g, (long)(q/g));
    } else add_work("No common factor greater than 1 remains.");
    fraction_text(p, q, answer, sizeof(answer));
    add_work("%s = %s", name, answer);
}

static void explain_check(void)
{
    char sum[40], product[40], expected_sum[40], expected_product[40];
    int32_t sp, sq, pp, pq;
    if (roots[0].radical) {
        int32_t s = roots[0].imaginary ? -1L : 1L;
        sp = 2L * roots[0].p;
        sq = roots[0].q;
        pp = roots[0].p * roots[0].p -
             s * roots[0].f * roots[0].f * roots[0].r;
        pq = roots[0].q * roots[0].q;
        add_work("Let u be the real part and v the added term.");
        add_work("The roots are u+v and u-v.");
        add_work("Their sum is (u+v)+(u-v) = 2u.");
        add_work("Their product is (u+v)(u-v) = u^2-v^2.");
        if (roots[0].imaginary) add_work("Use i^2 = -1 when squaring v.");
        add_work("2u = (2*%ld)/%ld = %ld/%ld.",
                 (long)roots[0].p, (long)roots[0].q, (long)sp, (long)sq);
        add_work("u^2-v^2 = [(%ld)^2-(%ld)*(%ld)^2*(%ld)]/(%ld)^2.",
                 (long)roots[0].p, (long)s, (long)roots[0].f,
                 (long)roots[0].r, (long)roots[0].q);
        add_work("This gives %ld/%ld.", (long)pp, (long)pq);
    } else {
        const root_t *r1 = &roots[0];
        const root_t *r2 = repeated ? &roots[0] : &roots[1];
        sp = r1->p*r2->q + r2->p*r1->q;
        sq = r1->q*r2->q;
        pp = r1->p*r2->p;
        pq = sq;
        add_work("Add the two roots using a common denominator:");
        add_work("sum = [(%ld)*(%ld)+(%ld)*(%ld)]/[(%ld)*(%ld)]",
                 (long)r1->p, (long)r2->q, (long)r2->p,
                 (long)r1->q, (long)r1->q, (long)r2->q);
        add_work("sum = %ld/%ld", (long)sp, (long)sq);
        add_work("Multiply their numerators and denominators:");
        add_work("product = [(%ld)*(%ld)]/[(%ld)*(%ld)] = %ld/%ld",
                 (long)r1->p, (long)r2->p, (long)r1->q,
                 (long)r2->q, (long)pp, (long)pq);
        if (repeated) add_work("For this check, count the repeated root twice.");
    }
    fraction_text(sp, sq, sum, sizeof(sum));
    fraction_text(pp, pq, product, sizeof(product));
    fraction_text(-coefficient_b, coefficient_a, expected_sum, sizeof(expected_sum));
    fraction_text(coefficient_c, coefficient_a, expected_product, sizeof(expected_product));
    add_work("Reduced sum = %s; -B/A = %s.", sum, expected_sum);
    add_work("Reduced product = %s; C/A = %s.", product, expected_product);
    if (strcmp(sum, expected_sum) || strcmp(product, expected_product))
        add_work("CHECK FAILED: do not rely on these roots.");
    else {
        add_work("A(x-r1)(x-r2) = A*x^2-A*(sum)*x+A*(product).");
        add_work("The coefficients equal A, B, C. Both roots check.");
    }
}

static void build_work(void)
{
    int32_t a = coefficient_a, b = coefficient_b, c = coefficient_c;
    int32_t d, m, s = 0, f = 1, r, i, p, q, g;
    unsigned j;
    char answer[112], hs[40], ks[40];
    work_count = 0;
    work_overflow = false;
    add_work("1. WRITE THE EQUATION IN STANDARD FORM");
    add_work("(%ld)*x^2 + (%ld)*x + (%ld) = 0", (long)a, (long)b, (long)c);
    add_work("A=%ld, B=%ld, C=%ld.", (long)a, (long)b, (long)c);
    add_work("Notation: sqrt(n) means the square root of n.");
    add_work("A fraction (numerator)/(denominator) divides the entire numerator.");
    if (a == 0) {
        add_work("2. A IS ZERO: THIS IS NOT QUADRATIC");
        if (b == 0) {
            add_work("The equation becomes %ld = 0.", (long)c);
            if (c == 0) add_work("0 = 0 is always true. Every x is a solution.");
            else add_work("%ld = 0 is false. There is no solution.", (long)c);
        } else {
            add_work("The x^2 term vanishes: (%ld)*x + (%ld) = 0.", (long)b, (long)c);
            add_work("Subtract %ld from both sides.", (long)c);
            add_work("(%ld)*x = %ld.", (long)b, (long)-c);
            add_work("Divide both sides by %ld, which is nonzero.", (long)b);
            explain_fraction("x", -c, b);
            add_work("CHECK BY SUBSTITUTION");
            add_work("B*x+C = (%ld)*(%ld/%ld)+(%ld)",
                     (long)b, (long)roots[0].p, (long)roots[0].q, (long)c);
            add_work("= [(%ld)*(%ld)+(%ld)*(%ld)]/%ld",
                     (long)b, (long)roots[0].p, (long)c,
                     (long)roots[0].q, (long)roots[0].q);
            add_work("= %ld/%ld = 0. The solution checks.",
                     (long)(b*roots[0].p+c*roots[0].q), (long)roots[0].q);
        }
        add_work("A parabola vertex, quadratic axis, and opening direction do not apply when A=0.");
        return;
    }
    d = b*b - 4L*a*c;
    m = abs32(d);
    while ((s+1)*(s+1) <= m) ++s;
    add_work("2. CALCULATE THE DISCRIMINANT");
    add_work("D = B^2 - 4*A*C");
    add_work("D = (%ld)^2 - 4*(%ld)*(%ld)", (long)b, (long)a, (long)c);
    add_work("B^2 = (%ld)*(%ld) = %ld.", (long)b, (long)b, (long)(b*b));
    add_work("4*A = 4*(%ld) = %ld.", (long)a, (long)(4L*a));
    add_work("4*A*C = (%ld)*(%ld) = %ld.", (long)(4L*a), (long)c, (long)(4L*a*c));
    add_work("D = %ld - (%ld) = %ld.", (long)(b*b), (long)(4L*a*c), (long)d);
    if (d < 0) add_work("D<0, so there are two nonreal complex roots.");
    else if (!d) add_work("D=0, so there is one repeated real root.");
    else if (s*s==d) add_work("D>0 and %ld^2=%ld, so both roots are rational real numbers.", (long)s, (long)d);
    else {
        add_work("%ld^2=%ld < %ld < %ld=(%ld)^2.",
                 (long)s, (long)(s*s), (long)d, (long)((s+1)*(s+1)), (long)(s+1));
        add_work("D>0 is not a perfect square, so both roots are irrational real numbers.");
    }
    add_work("3. SUBSTITUTE INTO THE QUADRATIC FORMULA");
    add_work("x = (-B +/- sqrt(D))/(2*A)");
    add_work("x = (-(%ld) +/- sqrt(%ld))/(2*(%ld))", (long)b, (long)d, (long)a);
    add_work("-B = -(%ld) = %ld.", (long)b, (long)-b);
    add_work("2*A = 2*(%ld) = %ld.", (long)a, (long)(2L*a));
    add_work("x = (%ld +/- sqrt(%ld))/(%ld)", (long)-b, (long)d, (long)(2L*a));
    add_work("4. SIMPLIFY THE SQUARE ROOT");
    if (d >= 0 && s*s == d) {
        add_work("sqrt(%ld) = %ld because %ld^2=%ld.",
                 (long)d, (long)s, (long)s, (long)d);
        add_work("5. SEPARATE AND REDUCE THE ROOTS");
        for (j=0; j<(s==0 ? 1U : 2U); ++j) {
            int32_t n = -b + (j==0 ? s : -s);
            const char *name = j==0 ? "x1" : "x2";
            add_work("%s uses the %s sign.", name, j==0 ? "plus" : "minus");
            add_work("Numerator: %ld %s %ld = %ld.", (long)-b,
                     j==0 ? "+" : "-", (long)s, (long)n);
            explain_fraction(name, n, 2L*a);
        }
        if (!s) add_work("The plus and minus cases agree because sqrt(D)=0. This is a repeated root.");
    } else {
        if (d < 0) {
            add_work("sqrt(%ld) = sqrt(-1)*sqrt(%ld).", (long)d, (long)m);
            add_work("Since i^2=-1, sqrt(D) = i*sqrt(%ld).", (long)m);
        }
        r=m;
        add_work("Start with sqrt(%ld). Extract perfect-square factors.", (long)r);
        for (i=2; i*i<=r; ++i) {
            while (r%(i*i)==0) {
                int32_t old=r, old_f=f;
                r/=i*i;
                f*=i;
                add_work("%ld = %ld^2 * %ld.", (long)old, (long)i, (long)r);
                add_work("sqrt(%ld) = %ld*sqrt(%ld).", (long)old, (long)i, (long)r);
                add_work("Outside factor: %ld*%ld = %ld.", (long)old_f, (long)i, (long)f);
            }
        }
        if (r==1) add_work("sqrt(1)=1, so the remaining radical disappears.");
        else add_work("%ld has no remaining square factor greater than 1.", (long)r);
        add_work("sqrt(%ld) = %ld*sqrt(%ld).", (long)m, (long)f, (long)r);
        if (d<0) add_work("Restore i: sqrt(D) = %ld*i*sqrt(%ld).", (long)f, (long)r);
        add_work("5. REDUCE THE WHOLE FRACTION");
        p=-b; q=2L*a;
        add_work("Real numerator=%ld; radical factor=%ld; denominator=%ld.",
                 (long)p, (long)f, (long)q);
        if (q<0) {
            p=-p; q=-q;
            add_work("Multiply numerator and denominator by -1 to make the denominator positive.");
            add_work("This interchanges the +/- choices; the solution set is unchanged.");
            add_work("Now the real numerator is %ld and denominator is %ld.", (long)p, (long)q);
        }
        g=gcd32(gcd32(p,f),q);
        add_work("The common factor of %ld, %ld, and %ld is %ld.",
                 (long)abs32(p), (long)f, (long)q, (long)g);
        add_work("Real numerator: %ld/%ld = %ld.", (long)p, (long)g, (long)(p/g));
        add_work("Radical factor: %ld/%ld = %ld.", (long)f, (long)g, (long)(f/g));
        add_work("Denominator: %ld/%ld = %ld.", (long)q, (long)g, (long)(q/g));
        add_work("Divide every numerator term and the denominator by the same factor.");
        if (!p) add_work("Omit the zero real term.");
        if (f/g==1) add_work("Omit the coefficient 1 on the radical or imaginary term.");
        if (q/g==1) add_work("A denominator of 1 can be omitted.");
        for (j=0;j<root_count;++j) {
            root_text(&roots[j],answer,sizeof(answer));
            add_work("x%u = %s",j+1,answer);
        }
    }
    add_work("6. CHECK BOTH ROOTS USING SUM AND PRODUCT");
    explain_check();
    add_work("7. FIND THE VERTEX AND AXIS OF THE FUNCTION");
    add_work("Now consider y=(%ld)*x^2+(%ld)*x+(%ld).", (long)a,(long)b,(long)c);
    add_work("The vertex x-coordinate is h=-B/(2*A).");
    explain_fraction("h",-b,2L*a);
    add_work("For the y-coordinate, use k=-D/(4*A).");
    add_work("-D = -(%ld) = %ld.", (long)d,(long)-d);
    add_work("4*A = 4*(%ld) = %ld.", (long)a,(long)(4L*a));
    explain_fraction("k",-d,4L*a);
    fraction_text(-b,2L*a,hs,sizeof(hs));
    fraction_text(-d,4L*a,ks,sizeof(ks));
    add_work("The exact vertex is (%s, %s).",hs,ks);
    add_work("The symmetry axis passes through the vertex: x = %s.",hs);
    add_work("8. WHY THE VERTEX FORMULAS WORK");
    add_work("Factor A from the x terms and complete the square:");
    add_work("y=A*[x^2+(B/A)*x]+C");
    add_work("Half of B/A is B/(2*A); square it to get B^2/(4*A^2).");
    add_work("Add and subtract that square inside the brackets:");
    add_work("y=A*[(x+B/(2*A))^2-B^2/(4*A^2)]+C");
    add_work("y=A*(x+B/(2*A))^2-B^2/(4*A)+C");
    add_work("Combine the constant terms over 4*A:");
    add_work("k=(-B^2+4*A*C)/(4*A)=-D/(4*A)");
    add_work("Numerically: k=[-(%ld)+(%ld)]/(%ld) = (%ld)/(%ld).",
             (long)(b*b),(long)(4L*a*c),(long)(4L*a),(long)-d,(long)(4L*a));
    add_work("Thus y=A*(x-h)^2+k with h=%s and k=%s.",hs,ks);
    add_work("9. OPENING DIRECTION AND EXTREME VALUE");
    if (a>0) {
        add_work("A=%ld>0. The squared term is nonnegative and is multiplied by a positive number.",(long)a);
        add_work("The parabola opens up. Its minimum value is k=%s at x=%s.",ks,hs);
    } else {
        add_work("A=%ld<0. Multiplying the nonnegative squared term by A makes it nonpositive.",(long)a);
        add_work("The parabola opens down. Its maximum value is k=%s at x=%s.",ks,hs);
    }
    add_work("END OF WORK. The next screen shows the formatted exact roots.");
}

/* true: return to answers; false: return to the suite menu. */
static bool show_work(void)
{
    unsigned current=0;
    unsigned pages=(work_count+WORK_PAGE-1)/WORK_PAGE;
    if (!pages) pages=1;
    for (;;) {
        unsigned i;
        uint8_t key;
        char text[64];
        page("ALGEBRA / STEP-BY-STEP WORK");
        snprintf(text,sizeof(text),"Page %u of %u",current+1,pages);
        gfx_PrintStringXY(text,12,43);
        for (i=0;i<WORK_PAGE;++i) {
            unsigned index=current*WORK_PAGE+i;
            if (index>=work_count) break;
            gfx_PrintStringXY(work_lines[index],12,63+(int)i*14);
        }
        gfx_HorizLine(12,207,296);
        gfx_PrintStringXY(work_overflow ? "WORK LIMIT REACHED: incomplete view"
            : "LEFT/UP Back   RIGHT/DOWN Next",12,218);
        gfx_PrintStringXY(current+1<pages ? "ENTER Next  GRAPH Roots  Y= Menu"
            : "ENTER Roots GRAPH Roots  Y= Menu",12,231);
        gfx_BlitBuffer();
        key=wait_key();
        if (key==sk_Yequ) return false;
        if (key==sk_Graph || key==sk_Clear) return true;
        if (key==sk_Up || key==sk_Left) { if (current) --current; }
        else if (key==sk_Down || key==sk_Right) {
            if (current+1<pages) ++current;
        } else if (key==sk_Enter) {
            if (current+1<pages) ++current;
            else return true;
        }
    }
}

static bool show_details(void)
{
    int32_t a=coefficient_a,b=coefficient_b,c=coefficient_c;
    for (;;) {
        char text[80];
        uint8_t key;
        page("ALGEBRA / EQUATION DETAILS");
        snprintf(text,sizeof(text),"A=%ld B=%ld C=%ld",(long)a,(long)b,(long)c);
        gfx_PrintStringXY(text,12,43);
        if (!a) {
            gfx_PrintStringXY(b ? "Linear equation (A=0)"
                : "Constant equation (A=B=0)",12,66);
            gfx_PrintStringXY("Quadratic discriminant: not applicable",12,90);
            gfx_PrintStringXY("Parabola vertex: not applicable",12,110);
            gfx_PrintStringXY("Symmetry axis: not applicable",12,130);
            gfx_PrintStringXY("Opening/min/max: not applicable",12,150);
            gfx_PrintStringXY(b ? "One rational solution" :
                (c==0 ? "Identity: every x is a solution" : "Contradiction: no solution"),12,178);
        } else {
            int32_t d=b*b-4L*a*c;
            root_t h=rational_root(-b,2L*a),k=rational_root(-d,4L*a);
            expression_t he,ke;
            int x;
            prepare_expression(&h,&he);
            prepare_expression(&k,&ke);
            snprintf(text,sizeof(text),"D = B^2 - 4AC = %ld",(long)d);
            gfx_PrintStringXY(text,12,64);
            gfx_PrintStringXY(d<0 ? "Two nonreal complex roots" :
                (d==0 ? "One repeated real root" :
                (roots[0].radical ? "Two irrational real roots" : "Two rational real roots")),12,82);
            gfx_PrintStringXY("Vertex:",12,118);
            gfx_PrintStringXY("(",72,118);
            x=84;
            draw_expression(&he,x,118);
            x+=he.width+5;
            gfx_PrintStringXY(",",x,118);
            x+=14;
            draw_expression(&ke,x,118);
            x+=ke.width+5;
            gfx_PrintStringXY(")",x,118);
            gfx_PrintStringXY("Axis: x =",12,162);
            draw_expression(&he,90,162);
            gfx_PrintStringXY(a>0 ? "Opens up; vertex is a minimum."
                : "Opens down; vertex is a maximum.",12,192);
        }
        gfx_HorizLine(12,207,296);
        gfx_PrintStringXY("ENTER/CLEAR/GRAPH: roots",12,218);
        gfx_PrintStringXY("TRACE: work       Y=: main menu",12,231);
        gfx_BlitBuffer();
        key=wait_key();
        if (key==sk_Enter || key==sk_Clear || key==sk_Graph) return true;
        if (key==sk_Yequ) return false;
        if (key==sk_Trace && !show_work()) return false;
    }
}

static bool show_results(void)
{
    expression_t expressions[2];
    unsigned i;
    bool fits=true;
    for (i=0;i<root_count;++i) {
        prepare_expression(&roots[i],&expressions[i]);
        if (expressions[i].width>296) fits=false;
    }
    for (;;) {
        char text[64];
        uint8_t key;
        page("ALGEBRA / EXACT SOLUTIONS");
        snprintf(text,sizeof(text),"A=%ld B=%ld C=%ld",
            (long)coefficient_a,(long)coefficient_b,(long)coefficient_c);
        gfx_PrintStringXY(text,12,44);
        if (!fits) gfx_PrintStringXY("Answer exceeds display width.",12,88);
        else if (!root_count) gfx_PrintStringXY(all_numbers
            ? "ALL COMPLEX NUMBERS" : "NO SOLUTION",12,88);
        else if (root_count==1) {
            draw_expression(&expressions[0],12,96);
            if (repeated) gfx_PrintStringXY("Repeated root",12,140);
        } else {
            int combined=expressions[0].width+expressions[1].width+24;
            if (combined<=296) {
                int x=12;
                draw_expression(&expressions[0],x,100);
                x+=expressions[0].width+6;
                gfx_PrintStringXY(",",x,100);
                draw_expression(&expressions[1],x+18,100);
            } else {
                draw_expression(&expressions[0],12,86);
                if (expressions[0].width<=282)
                    gfx_PrintStringXY(",",18+expressions[0].width,86);
                else gfx_PrintStringXY(",",12,116);
                draw_expression(&expressions[1],12,150);
            }
        }
        gfx_HorizLine(12,195,296);
        gfx_PrintStringXY("ENTER/CLEAR: edit coefficients",12,207);
        gfx_PrintStringXY("GRAPH: details   TRACE: work",12,220);
        gfx_PrintStringXY("Y=: main menu",12,232);
        gfx_BlitBuffer();
        key=wait_key();
        if (key==sk_Enter || key==sk_Clear) return true;
        if (key==sk_Yequ) return false;
        if (key==sk_Graph && !show_details()) return false;
        if (key==sk_Trace && !show_work()) return false;
    }
}

static void run_quadratic_module(void)
{
    unsigned selected=0;
    for (;;) {
        if (!input_coefficients(&selected)) return;
        solve();
        build_work();
        if (!show_work()) return;
        if (!show_results()) return;
        selected=0;
    }
}

static void show_about(void)
{
    uint8_t key;
    page("ALGEBRA SUITE / ABOUT");
    gfx_PrintStringXY("Version " APP_VERSION,12,44);
    gfx_PrintStringXY("Quadratic and linear equations",12,67);
    gfx_PrintStringXY("Automatic step-by-step work",12,87);
    gfx_PrintStringXY("Exact fractions, radicals, complex roots",12,107);
    gfx_PrintStringXY("Vertex, axis, classification, checks",12,127);
    gfx_PrintStringXY("Integer inputs: -1000 to 1000",12,150);
    gfx_PrintStringXY("Work exists only in this session.",12,171);
    gfx_PrintStringXY("No persistent logs or saved inputs.",12,189);
    gfx_HorizLine(12,207,296);
    gfx_PrintStringXY("ENTER, CLEAR, or Y=: main menu",12,220);
    gfx_BlitBuffer();
    do { key=wait_key(); }
    while (key!=sk_Enter && key!=sk_Clear && key!=sk_Yequ);
}

static unsigned main_menu(unsigned selected)
{
    static const char *items[]={"Quadratic / linear solver","About","Exit"};
    for (;;) {
        unsigned i;
        uint8_t key;
        page("ALGEBRA SUITE");
        gfx_PrintStringXY("Honors Algebra 2 project",12,44);
        for (i=0;i<3;++i) {
            int y=82+(int)i*34;
            if (i==selected) {
                gfx_Rectangle(8,y-8,304,27);
                gfx_PrintStringXY(">",16,y);
            }
            gfx_PrintStringXY(items[i],34,y);
        }
        gfx_PrintStringXY("Version " APP_VERSION,12,185);
        gfx_HorizLine(12,203,296);
        gfx_PrintStringXY("UP/DOWN: select   ENTER: open",12,214);
        gfx_PrintStringXY("CLEAR or Y=: quit",12,228);
        gfx_BlitBuffer();
        key=wait_key();
        if (key==sk_Up) selected=(selected+2)%3;
        else if (key==sk_Down) selected=(selected+1)%3;
        else if (key==sk_Enter) return selected;
        else if (key==sk_Clear || key==sk_Yequ) return 2;
    }
}

int main(void)
{
    unsigned selection=0;
    bool running=true;
    begin_screen();
    while (running) {
        selection=main_menu(selection);
        switch (selection) {
            case 0: run_quadratic_module(); break;
            case 1: show_about(); break;
            default: running=false; break;
        }
    }
    gfx_End();
    return 0;
}
