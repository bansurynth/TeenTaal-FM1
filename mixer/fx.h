/* fx.h — ECHO, DELAY and REVERB: three sends on the mixer's output (after MASTER), each added to the dry sum.
 * A send at 0 passes the signal untouched (bit-exact), so the effects cost nothing until they are used.
 *
 * Parameters (fx_set_param, target ENG_FX), all 0..100:
 *   FX_P_ECHO / FX_P_REVERB / FX_P_DELAY        send level, on a squared curve (default 0: off)
 *   FX_P_ECHO_TIME      120..400 ms        FX_P_ECHO_FB     feedback 0..90 %
 *   FX_P_REVERB_SIZE    feedback 0.70..0.98
 *   FX_P_DELAY_TIME     5..150 ms          FX_P_DELAY_FB    feedback 0..50 %
 * The delay lines and the reverb's combs and allpasses store Q23 samples in int32. */
#pragma once
#include <stdint.h>
#include "dsp/dsp.h"

enum {
    FX_P_ECHO = DSP_P_MODULE, FX_P_ECHO_TIME, FX_P_ECHO_FB,
    FX_P_REVERB, FX_P_REVERB_SIZE,
    FX_P_DELAY, FX_P_DELAY_TIME, FX_P_DELAY_FB
};
#define FX_NPARAM (FX_P_DELAY_FB - DSP_P_MODULE + 1)
enum { FX_S_ECHO, FX_S_REVERB, FX_S_DELAY, FX_NSEND };

#define FX_ECHO_CAP 17652               /* 400 ms at DSP_FS (17647 samples) plus margin */
#define FX_DELAY_CAP 6624               /* 150 ms at DSP_FS (6618 samples) plus margin */
#define FX_RV_NC 8                      /* Freeverb's combs */
#define FX_RV_NL 12                     /* the combs, then 4 allpasses */
#define FX_RV_TOTAL (1116 + 1188 + 1277 + 1356 + 1422 + 1491 + 1557 + 1617 + 556 + 441 + 341 + 225)

/* a delay line's read position and loop state */
typedef struct {
    int32_t w;                          /* the slot to write next */
    float d, dt;                        /* the delay now and its target, in samples */
    float lp;                           /* the loop's damping filter */
} fx_line_t;

typedef struct {
    int32_t p[FX_NPARAM];               /* the parameters as set */
    float gain[FX_NSEND];               /* the sends now, after the last block */
    int32_t cleared[FX_NSEND];          /* 1 once a send is off and its buffers are cleared */
    uint32_t clips;                     /* samples clipped at full scale since init */
    fx_line_t echo, delay;
    int32_t rv_off[FX_RV_NL], rv_pos[FX_RV_NL];
    float rv_fs[FX_RV_NC];              /* the combs' damping states */
    int32_t echo_buf[FX_ECHO_CAP];
    int32_t delay_buf[FX_DELAY_CAP];
    int32_t rv_buf[FX_RV_TOTAL];
} fx_t;

void fx_init(fx_t *x);
void fx_set_param(fx_t *x, int param_id, int32_t value);
int32_t fx_get_param(const fx_t *x, int param_id);
/* lr: n stereo frames of the mixer's output (both channels equal, Q23), overwritten; n <= DSP_BLOCK */
void fx_process(fx_t *x, int32_t *lr, int n);
