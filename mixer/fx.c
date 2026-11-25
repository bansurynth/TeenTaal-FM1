/* fx.c — see fx.h */
#include "mixer/fx.h"
#include "dsp/ramp.h"

#define FX_ECHO_DAMP 0.6f               /* one-pole lowpass in the echo loop, about 6 kHz */
#define FX_DELAY_DAMP 1.0f              /* the delay loop has no filter */
#define FX_GLIDE 0.5f                   /* the delay moves toward a new time at most this many samples per sample */
#define FX_RV_IN 0.1f                   /* reverb input gain, so the combs stay below full scale */
#define FX_RV_OUT (0.5f / FX_RV_IN)     /* reverb output gain at full send (calibrated by measurement) */
#define FX_RV_DAMP 0.2f                 /* Freeverb's damping at its default (0.5 * 0.4) */
#define FX_RV_AP_FB 0.5f                /* Freeverb's allpass feedback */

static const int FX_RV_LEN[FX_RV_NL] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617, 556, 441, 341, 225};
static const int FX_SEND_IX[FX_NSEND] = {0, 3, 5};             /* the send's level, as an index into p[] */

static float fx_load(int32_t v) { return (float)v; }

static int32_t fx_store(float q)
{
    if (q >= (float)DSP_Q23_MAX)
        return DSP_Q23_MAX;
    if (q <= -(float)DSP_Q23_MAX)
        return -DSP_Q23_MAX;
    return (int32_t)(q >= 0.0f ? q + 0.5f : q - 0.5f);
}

static float level_gain(int32_t pct)
{
    float v = (float)pct * 0.01f;
    return v * v;
}

static void fx_clear32(int32_t *b, int n)
{
    int i;
    for (i = 0; i < n; i++)
        b[i] = 0;
}

static int32_t clamp100(int32_t v) { return v < 0 ? 0 : v > 100 ? 100 : v; }

void fx_init(fx_t *x)
{
    int k, off = 0;
    for (k = 0; k < FX_NPARAM; k++)
        x->p[k] = 0;
    x->p[FX_P_ECHO_TIME - DSP_P_MODULE] = 40;
    x->p[FX_P_ECHO_FB - DSP_P_MODULE] = 40;
    x->p[FX_P_REVERB_SIZE - DSP_P_MODULE] = 50;
    x->p[FX_P_DELAY_TIME - DSP_P_MODULE] = 20;
    x->p[FX_P_DELAY_FB - DSP_P_MODULE] = 30;
    for (k = 0; k < FX_NSEND; k++) {
        x->gain[k] = 0.0f;
        x->cleared[k] = 1;
    }
    x->clips = 0;
    x->echo.w = x->delay.w = 0;
    x->echo.d = x->echo.dt = x->delay.d = x->delay.dt = 0.0f;
    x->echo.lp = x->delay.lp = 0.0f;
    for (k = 0; k < FX_RV_NL; k++) {
        x->rv_off[k] = off;
        x->rv_pos[k] = 0;
        off += FX_RV_LEN[k];
    }
    for (k = 0; k < FX_RV_NC; k++)
        x->rv_fs[k] = 0.0f;
    fx_clear32(x->echo_buf, FX_ECHO_CAP);
    fx_clear32(x->delay_buf, FX_DELAY_CAP);
    fx_clear32(x->rv_buf, FX_RV_TOTAL);
}

void fx_set_param(fx_t *x, int id, int32_t v)
{
    if (id >= FX_P_ECHO && id < FX_P_ECHO + FX_NPARAM)
        x->p[id - DSP_P_MODULE] = clamp100(v);
}

int32_t fx_get_param(const fx_t *x, int id)
{
    return id >= FX_P_ECHO && id < FX_P_ECHO + FX_NPARAM ? x->p[id - DSP_P_MODULE] : 0;
}

/* one sample of a delay line: returns the delayed signal; writes in + feedback * loop into the line */
static float fx_line_step(int32_t *buf, int cap, fx_line_t *L, float in, float fb, float damp)
{
    int di, i0, i1;
    float f, y;
    if (L->d < L->dt) {
        L->d += FX_GLIDE;
        if (L->d > L->dt)
            L->d = L->dt;
    } else if (L->d > L->dt) {
        L->d -= FX_GLIDE;
        if (L->d < L->dt)
            L->d = L->dt;
    }
    di = (int)L->d;
    f = L->d - (float)di;
    i0 = L->w - di;
    if (i0 < 0)
        i0 += cap;
    i1 = i0 - 1;
    if (i1 < 0)
        i1 += cap;
    y = (1.0f - f) * fx_load(buf[i0]) + f * fx_load(buf[i1]);
    L->lp += damp * (y - L->lp);
    buf[L->w] = fx_store(in + fb * L->lp);
    L->w = L->w + 1 < cap ? L->w + 1 : 0;
    return y;
}

/* one sample of Freeverb (mono): 8 combs in parallel, then 4 allpasses in series; in is the dry signal (Q23) */
static float fx_reverb_step(fx_t *x, float in, float fb)
{
    float sum = 0.0f, s = in * FX_RV_IN;
    int c, a;
    for (c = 0; c < FX_RV_NC; c++) {
        int32_t *b = x->rv_buf + x->rv_off[c];
        int32_t p = x->rv_pos[c];
        float o = fx_load(b[p]);
        x->rv_fs[c] = o * (1.0f - FX_RV_DAMP) + x->rv_fs[c] * FX_RV_DAMP;
        b[p] = fx_store(s + x->rv_fs[c] * fb);
        x->rv_pos[c] = p + 1 < FX_RV_LEN[c] ? p + 1 : 0;
        sum += o;
    }
    for (a = FX_RV_NC; a < FX_RV_NL; a++) {
        int32_t *b = x->rv_buf + x->rv_off[a];
        int32_t p = x->rv_pos[a];
        float bo = fx_load(b[p]), y;
        y = bo - sum;
        b[p] = fx_store(sum + bo * FX_RV_AP_FB);
        x->rv_pos[a] = p + 1 < FX_RV_LEN[a] ? p + 1 : 0;
        sum = y;
    }
    return sum * FX_RV_OUT;
}

void fx_process(fx_t *x, int32_t *lr, int n)
{
    float g0[FX_NSEND], dg[FX_NSEND], inv_n = 1.0f / (float)n;
    float echo_fb, delay_fb, rv_fb;
    int act[FX_NSEND], any = 0, k, i;
    for (k = 0; k < FX_NSEND; k++) {
        act[k] = x->p[FX_SEND_IX[k]] > 0 || x->gain[k] > 0.0f;
        any |= act[k];
    }
    for (k = 0; k < FX_NSEND; k++) {    /* the sends ramp like the mixer's: 20 ms a full sweep */
        float g1 = dsp_slew(x->gain[k], level_gain(x->p[FX_SEND_IX[k]]), n);
        g0[k] = x->gain[k];
        dg[k] = (g1 - g0[k]) * inv_n;
        x->gain[k] = g1;
    }
    echo_fb = 0.9f * (float)x->p[FX_P_ECHO_FB - DSP_P_MODULE] * 0.01f;
    delay_fb = 0.5f * (float)x->p[FX_P_DELAY_FB - DSP_P_MODULE] * 0.01f;
    rv_fb = 0.70f + 0.28f * (float)x->p[FX_P_REVERB_SIZE - DSP_P_MODULE] * 0.01f;
    x->echo.dt = (120.0f + 2.8f * (float)x->p[FX_P_ECHO_TIME - DSP_P_MODULE]) * (DSP_FS * 0.001f);
    x->delay.dt = (5.0f + 1.45f * (float)x->p[FX_P_DELAY_TIME - DSP_P_MODULE]) * (DSP_FS * 0.001f);

    /* an off send: its buffers are cleared once, so an old echo does not return when it is switched on again */
    if (act[FX_S_ECHO]) {
        x->cleared[FX_S_ECHO] = 0;
    } else {
        x->echo.d = x->echo.dt;
        x->echo.lp = 0.0f;
        if (!x->cleared[FX_S_ECHO]) {
            fx_clear32(x->echo_buf, FX_ECHO_CAP);
            x->cleared[FX_S_ECHO] = 1;
        }
    }
    if (act[FX_S_DELAY]) {
        x->cleared[FX_S_DELAY] = 0;
    } else {
        x->delay.d = x->delay.dt;
        x->delay.lp = 0.0f;
        if (!x->cleared[FX_S_DELAY]) {
            fx_clear32(x->delay_buf, FX_DELAY_CAP);
            x->cleared[FX_S_DELAY] = 1;
        }
    }
    if (act[FX_S_REVERB]) {
        x->cleared[FX_S_REVERB] = 0;
    } else if (!x->cleared[FX_S_REVERB]) {
        fx_clear32(x->rv_buf, FX_RV_TOTAL);
        for (k = 0; k < FX_RV_NC; k++)
            x->rv_fs[k] = 0.0f;
        for (k = 0; k < FX_RV_NL; k++)
            x->rv_pos[k] = 0;
        x->cleared[FX_S_REVERB] = 1;
    }
    if (!any)                           /* every send off: the mixer's output passes untouched */
        return;

    for (i = 0; i < n; i++) {
        float t = (float)(i + 1), dry = (float)lr[2 * i], acc = dry, y;
        if (act[FX_S_ECHO]) {
            y = fx_line_step(x->echo_buf, FX_ECHO_CAP, &x->echo, dry, echo_fb, FX_ECHO_DAMP);
            acc += (g0[FX_S_ECHO] + dg[FX_S_ECHO] * t) * y;
        }
        if (act[FX_S_DELAY]) {
            y = fx_line_step(x->delay_buf, FX_DELAY_CAP, &x->delay, dry, delay_fb, FX_DELAY_DAMP);
            acc += (g0[FX_S_DELAY] + dg[FX_S_DELAY] * t) * y;
        }
        if (act[FX_S_REVERB]) {
            y = fx_reverb_step(x, dry, rv_fb);
            acc += (g0[FX_S_REVERB] + dg[FX_S_REVERB] * t) * y;
        }
        if (acc > DSP_Q23_F) {
            acc = DSP_Q23_F;
            x->clips++;
        } else if (acc < -DSP_Q23_F) {
            acc = -DSP_Q23_F;
            x->clips++;
        }
        lr[2 * i] = lr[2 * i + 1] = (int32_t)acc;
    }
}
