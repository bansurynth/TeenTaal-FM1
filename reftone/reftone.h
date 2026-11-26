/* reftone.h — reference tone: a steady, clean sine on a swara, tuned like the tanpura.
 *
 * Parameters (reftone_set_param): the shared tuning (DSP_P_A4_DHZ, DSP_P_SA_CENTS, DSP_P_TEMPERAMENT) and
 *   REFTONE_P_ON      0 / 1        on fades in, off fades out (20 ms)
 *   REFTONE_P_SWARA   0..11        SW_SA .. SW_NI (dsp/tuning.h), from Sa
 *   REFTONE_P_NOTE    -1900..700   a fixed note: cents above C4 (F2 .. G4), equal temperament, whatever Sa is;
 *                                  REFTONE_NOTE_SWARA: no fixed note, the tone is the swara
 *   REFTONE_P_OCTAVE  -3..+3       octaves: from Sa for the swara, from the note for a fixed note (OCT-/+)
 *   REFTONE_P_VOLUME  0..100       percent, on a squared curve
 * Changes never click: the gain moves in linear ramps, the phase is continuous. */
#pragma once
#include <stdint.h>
#include "dsp/dsp.h"
#include "dsp/tuning.h"

enum {
    REFTONE_P_ON = DSP_P_MODULE,
    REFTONE_P_SWARA,
    REFTONE_P_OCTAVE,
    REFTONE_P_VOLUME,
    REFTONE_P_NOTE
};

#define REFTONE_LEVEL 0.5f             /* volume 100 % = -6 dBFS before the mixer; fades: dsp/ramp.h */
#define REFTONE_OCT_MIN (-3)
#define REFTONE_OCT_MAX 3
#define REFTONE_NOTE_MIN (-1900)       /* F2 */
#define REFTONE_NOTE_MAX 700           /* G4 */
#define REFTONE_NOTE_SWARA INT32_MIN   /* REFTONE_P_NOTE off: the swara plays */

typedef struct {
    tuning_t tuning;
} reftone_config_t;

typedef struct {
    tuning_t tuning;
    int32_t on, swara, octave, volume, note;
    volatile uint32_t inc;             /* written by set_param (UI), read by render (audio interrupt) */
    uint32_t phase;
    float gain;                        /* the gain now, linear (0..REFTONE_LEVEL) */
} reftone_t;

void reftone_init(reftone_t *m, const reftone_config_t *cfg);   /* cfg NULL: default tuning */
void reftone_set_param(reftone_t *m, int param_id, int32_t value);
int32_t reftone_get_param(const reftone_t *m, int param_id);
void reftone_render(reftone_t *m, int32_t *out, int n_samples);   /* adds into out */
float reftone_hz(const reftone_t *m);                            /* the frequency it plays now */
