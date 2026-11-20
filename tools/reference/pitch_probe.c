/* Render one actual tanpura string through its body for offline pitch calibration.
 * The binary output is float32: target Hz, applied correction in cents, sample rate, then mono samples.
 *
 * pitch_probe SA REG JAWARI first|sa|kharaj|preview SELECTION FINE A4_DHZ auto|CORRECTION SECONDS OFFSET */
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tanpura/parameters.h"

static int parse_int(const char *text, int *out)
{
    char *end;
    long value;
    errno = 0;
    value = strtol(text, &end, 10);
    if (errno || end == text || *end || value < INT_MIN || value > INT_MAX)
        return 0;
    *out = (int)value;
    return 1;
}

static int parse_real(const char *text, double *out)
{
    char *end;
    errno = 0;
    *out = strtod(text, &end);
    return !errno && end != text && !*end && isfinite(*out);
}

int main(int argc, char **argv)
{
    static tanpura_t t;
    tstring_par_t p;
    tstring_coefs_t coefs;
    int32_t out[DSP_BLOCK];
    float buffer[DSP_BLOCK], header[3];
    int sa, reg, jawari, selection, fine, a4, string, note, i, pos, count;
    double target, correction = 0.0, seconds, offset;

    if (argc != 11) {
        fprintf(stderr, "usage: pitch_probe SA REG JAWARI first|sa|kharaj|preview "
                        "SELECTION FINE A4_DHZ auto|CORRECTION SECONDS OFFSET\n");
        return 2;
    }
    if (!parse_int(argv[1], &sa) || !parse_int(argv[2], &reg) || !parse_int(argv[3], &jawari) ||
        !parse_int(argv[5], &selection) || !parse_int(argv[6], &fine) || !parse_int(argv[7], &a4) ||
        (strcmp(argv[8], "auto") && !parse_real(argv[8], &correction)) || !parse_real(argv[9], &seconds) ||
        !parse_real(argv[10], &offset) || sa < TUNING_SA_MIN || sa > TUNING_SA_MAX || reg < 0 || reg > 1 ||
        jawari < 0 || jawari > 100 || fine < -50 || fine > 50 || a4 < TUNING_A4_DHZ_MIN ||
        a4 > TUNING_A4_DHZ_MAX || seconds <= 0.0 || seconds > 60.0 || fabs(offset) > 300.0 ||
        fabs(correction) > 300.0) {
        fprintf(stderr, "pitch_probe: invalid argument or out-of-range value\n");
        return 2;
    }
    count = (int)(seconds * DSP_FS);
    tanpura_init(&t, 0);
    tanpura_set_param(&t, DSP_P_A4_DHZ, a4);
    tanpura_set_param(&t, DSP_P_SA_CENTS, sa);
    tanpura_set_param(&t, TANPURA_P_REGISTER, reg);
    tanpura_set_param(&t, TANPURA_P_JAWARI, jawari);
    tanpura_set_param(&t, TANPURA_P_HUMAN, 0);
    note = tanpura_get_param(&t, TANPURA_P_SA_NOW);
    if (!strcmp(argv[4], "first")) {
        if (selection < 0 || selection > TANPURA_FIRST_MAX) {
            fprintf(stderr, "pitch_probe: invalid first-string swara\n");
            return 2;
        }
        string = 0;
        tanpura_set_param(&t, TANPURA_P_FIRST, selection);
        tanpura_set_param(&t, TANPURA_P_FINE, fine);
        note += 100 * selection - 1200 + fine;
        tanpura_string_par(&t, string, &p);
    } else if (!strcmp(argv[4], "sa") || !strcmp(argv[4], "kharaj")) {
        string = !strcmp(argv[4], "sa") ? 1 : 3;
        tanpura_set_param(&t, TANPURA_P_FINE + string, fine);
        note += fine - (string == 3 ? 1200 : 0);
        tanpura_string_par(&t, string, &p);
    } else if (!strcmp(argv[4], "preview")) {
        string = 1;
        note = selection;
        tanpura_preview_par(&t, note, &p);
    } else {
        fprintf(stderr, "pitch_probe: unknown string profile %s\n", argv[4]);
        return 2;
    }
    target = (double)a4 * 0.1 * pow(2.0, ((double)note - 900.0) / 1200.0);
    if (!strcmp(argv[8], "auto"))
        correction = 1200.0 * log2(target / (double)p.f1);
    target *= pow(2.0, offset / 1200.0);
    if (strcmp(argv[8], "auto") || offset != 0.0)
        p.f1 = tuning_hz(&t.tuning, (float)(note - sa) + (float)offset - (float)correction);

    tanpura_set_param(&t, TANPURA_P_ON, 1);
    tanpura_take(&t);
    for (i = 0; i < TANPURA_NSTR; i++) {
        tstring_silence(&t.str[i]);
        t.due[i] = i == string ? 0 : INT_MAX / 2;
        if (i != string)
            t.str[i].c.M = 0;
    }
    t.nominal[string] = 0;
    tstring_coefs(&coefs, &p, TANPURA_FS);
    tstring_init(&t.str[string], &coefs);
    t.gain = 1.0f;
    header[0] = (float)target;
    header[1] = (float)correction;
    header[2] = DSP_FS;
    if (fwrite(header, sizeof header[0], 3, stdout) != 3)
        return 1;
    for (pos = 0; pos < count; pos += DSP_BLOCK) {
        int n = count - pos < DSP_BLOCK ? count - pos : DSP_BLOCK;
        if (n & 1)
            n--;
        if (!n)
            break;
        memset(out, 0, sizeof out);
        tanpura_render(&t, out, n);
        for (i = 0; i < n; i++)
            buffer[i] = (float)out[i] * (1.0f / 8388608.0f);
        if (fwrite(buffer, sizeof buffer[0], (size_t)n, stdout) != (size_t)n)
            return 1;
    }
    return 0;
}
