/* tuning.c — see tuning.h */
#include "dsp/tuning.h"
#include "dsp/dsp.h"
#include "fastmath.h"

/* Swara intervals above Sa, in cents. EQUAL (the default): 100 cents a semitone (ElectroNick, 2026-11-06; DECISIONS.md).
 * JUST: the 12 swaras of a saptak in the 5-limit or in the Pythagorean ratios (ElectroNick's TEMPERAMENT option, step 8);
 * the octave above adds 1200 cents. */
static const char *const SWARA_NAME[SW_N] = {"Sa", "re", "Re", "ga", "Ga", "Ma", "Ma'", "Pa", "dha", "Dha", "ni", "Ni"};
static const float JUST5_CENTS[SW_N] = {0.0f, 111.731f, 203.910f, 315.641f, 386.314f, 498.045f,
                                        590.224f, 701.955f, 813.686f, 884.359f, 1017.596f, 1088.269f};   /* 1, 16/15, 9/8, 6/5, 5/4, 4/3, 45/32, 3/2, 8/5, 5/3, 9/5, 15/8 */
static const float JUSTPYTH_CENTS[SW_N] = {0.0f, 90.225f, 203.910f, 294.135f, 407.820f, 498.045f,
                                           611.730f, 701.955f, 792.180f, 905.865f, 996.090f, 1109.775f}; /* 1, 256/243, 9/8, 32/27, 81/64, 4/3, 729/512, 3/2, 128/81, 27/16, 16/9, 243/128 */

static int32_t clampi(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }

void tuning_default(tuning_t *t)
{
    t->a4_dhz = TUNING_A4_DHZ_DEFAULT;
    t->sa_cents = TUNING_SA_DEFAULT;
    t->temperament = TUNING_TEMPERAMENT_EQUAL;
}

int tuning_set_param(tuning_t *t, int id, int32_t v)
{
    if (id == DSP_P_A4_DHZ)
        t->a4_dhz = clampi(v, TUNING_A4_DHZ_MIN, TUNING_A4_DHZ_MAX);
    else if (id == DSP_P_SA_CENTS)
        t->sa_cents = clampi(v, TUNING_SA_MIN, TUNING_SA_MAX);
    else if (id == DSP_P_TEMPERAMENT)
        t->temperament = clampi(v, TUNING_TEMPERAMENT_EQUAL, TUNING_TEMPERAMENT_N - 1);
    else
        return 0;
    return 1;
}

float tuning_swara_cents(const tuning_t *t, int swara)
{
    int s = clampi(swara, 0, TUNING_SWARA_MAX), k = s % SW_N, octave = s / SW_N;
    if (t->temperament == TUNING_TEMPERAMENT_JUST5)
        return JUST5_CENTS[k] + 1200.0f * (float)octave;
    if (t->temperament == TUNING_TEMPERAMENT_JUSTPYTH)
        return JUSTPYTH_CENTS[k] + 1200.0f * (float)octave;
    return 100.0f * (float)s;
}

const char *tuning_swara_name(int swara)
{
    return SWARA_NAME[swara < 0 ? 0 : swara >= SW_N ? SW_N - 1 : swara];
}

float tuning_hz(const tuning_t *t, float cents)
{
    float a4 = (float)t->a4_dhz * 0.1f;
    return a4 * fm_exp2f(((float)(t->sa_cents - 900) + cents) * (1.0f / 1200.0f));
}

uint32_t tuning_phase_inc(float hz)
{
    float inc = hz * (4294967296.0f / DSP_FS);
    if (!(inc > 0.0f) || inc >= 2147483520.0f)  /* at or above Nyquist (or nonsense): silence */
        return 0;
    return (uint32_t)(int32_t)inc;              /* < 2^31: the signed conversion every FPU has */
}
