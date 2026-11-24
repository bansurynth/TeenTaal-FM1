/* tuning.h — Sa, swaras and frequencies, shared by tanpura, tabla and the reference tone.
 *
 * A pitch is given in cents above Sa. Sa is in cents above C4 at an explicit reference A4,
 * so: f = A4 * 2^((sa_cents - 900 + cents_above_sa) / 1200).
 * Called on parameter changes only, never per sample. */
#pragma once
#include <stdint.h>

#define TUNING_A4_DHZ_MIN 4150      /* 415.0 Hz */
#define TUNING_A4_DHZ_MAX 4660      /* 466.0 Hz */
#define TUNING_A4_DHZ_DEFAULT 4400
#define TUNING_SA_MIN (-1800)       /* F#2, ~92.5 Hz at A440 */
#define TUNING_SA_MAX 700           /* G4, ~392 Hz at A440 */
#define TUNING_SA_DEFAULT (-300)    /* A3, 220 Hz: the tanpura's Sa for an A bansuri (Sa A4), an octave below */

/* the twelve swaras of a saptak, from Sa */
enum { SW_SA, SW_RE_K, SW_RE, SW_GA_K, SW_GA, SW_MA, SW_MA_T, SW_PA, SW_DHA_K, SW_DHA, SW_NI_K, SW_NI, SW_N };
#define TUNING_SWARA_MAX 17         /* the tanpura's range: SW_SA .. the upper Sa (12) .. the upper Ma (17) */

/* the swara intervals (DSP_P_TEMPERAMENT): equal, or just intonation in the 5-limit or Pythagorean ratios */
enum { TUNING_TEMPERAMENT_EQUAL, TUNING_TEMPERAMENT_JUST5, TUNING_TEMPERAMENT_JUSTPYTH, TUNING_TEMPERAMENT_N };

typedef struct {
    int32_t a4_dhz;
    int32_t sa_cents;
    int32_t temperament;
} tuning_t;

void tuning_default(tuning_t *t);
/* apply a shared parameter (DSP_P_A4_DHZ, DSP_P_SA_CENTS, DSP_P_TEMPERAMENT), clamped; 1 if it was one of them */
int tuning_set_param(tuning_t *t, int id, int32_t v);
/* the swara's interval above Sa in cents, for the temperament of t: swara 0..TUNING_SWARA_MAX (12..17: the octave above) */
float tuning_swara_cents(const tuning_t *t, int swara);
/* frequency in Hz of a pitch `cents` above Sa */
float tuning_hz(const tuning_t *t, float cents);
/* 32-bit phase increment per sample for a frequency (0 above Nyquist) */
uint32_t tuning_phase_inc(float hz);
/* short name of a swara ("Sa", "re", "Re", ... lower case = komal, "Ma'" = tivra) */
const char *tuning_swara_name(int swara);
