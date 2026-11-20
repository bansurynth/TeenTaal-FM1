/* tanpura.c — see tanpura.h */
#include "tanpura/tanpura.h"
#include "tanpura/body_coefs.h"
#include "tanpura/upsample_coefs.h"
#include "fastmath.h"

static int32_t reg_offset(const tanpura_t *m);   /* (the register: below) */

#define TANPURA_LEVEL 0.151f       /* body output -> full scale: RMS about -23.5 dBFS after LEVEL_DB, the worst peak -1.4 */

/* loudness compensation (dB to add), measured by tools/reference/level_grid.py (RMS over 3 cycles) so that neither
 * Sa nor the jawari changes the loudness: rows Sa (cents above C4), columns jawari 0, 25, 50, 75, 100 */
static const float LEVEL_SA[6] = {-1800.0f, -1500.0f, -1000.0f, -300.0f, 0.0f, 700.0f};
static const float LEVEL_DB[6][5] = {
    {-0.5f, 0.9f, 2.1f, 3.7f, 5.9f},
    {-1.1f, -0.6f, 0.0f, 1.8f, 3.9f},
    {-0.4f, 0.9f, 1.6f, 2.1f, 3.1f},
    {1.5f, 1.9f, 2.3f, 2.4f, 2.8f},
    {1.8f, 2.2f, 3.0f, 3.7f, 4.0f},
    {3.0f, 4.6f, 4.5f, 4.6f, 5.3f},          /* G4, jawari 0: +3.0, not the measured +6.1: its peaks stand 22 dB over its
                                                RMS, and with the irregularity they clipped (tools/test/tanpura_stress) */
};

/* Contact-induced pitch shift in cents to subtract from the free-string target, measured by
 * tools/reference/pitch_grid.py. Note profiles: steel at -1200, -500, 0 and +500 cents from Sa;
 * shortened upper steel at Sa^ and Ma^; bronze kharaj at Sa-1200. Rows are effective instrument Sa,
 * columns jawari 0, 25, 50, 75, 100. */
#define PITCH_SA_N 9
#define PITCH_JAWARI_N 5
#define PITCH_NORMAL_N 4
static const float PITCH_SA[PITCH_SA_N] = {-1800.0f, -1500.0f, -1200.0f, -1000.0f, -800.0f, -500.0f, -300.0f, 0.0f, 700.0f};
static const float PITCH_NORMAL_NOTE[PITCH_NORMAL_N] = {-1200.0f, -500.0f, 0.0f, 500.0f};
static const float PITCH_NORMAL[PITCH_NORMAL_N][PITCH_SA_N][PITCH_JAWARI_N] = {
    {   /* normal steel, Sa below by an octave */
        {19.1f, 19.6f, 20.2f, 20.7f, 21.2f},
        {18.3f, 18.9f, 19.4f, 20.0f, 20.5f},
        {18.5f, 19.3f, 20.0f, 20.6f, 21.2f},
        {19.0f, 19.9f, 20.7f, 21.4f, 21.9f},
        {19.8f, 20.7f, 21.5f, 22.3f, 22.7f},
        {21.3f, 22.2f, 23.2f, 24.1f, 24.5f},
        {22.7f, 23.7f, 24.7f, 25.6f, 26.2f},
        {24.9f, 26.2f, 27.2f, 28.2f, 29.0f},
        {29.1f, 30.7f, 32.3f, 33.6f, 35.3f},
    },
    {   /* normal steel, Pa */
        {17.5f, 18.3f, 19.0f, 19.6f, 20.2f},
        {17.7f, 18.6f, 19.3f, 19.8f, 20.5f},
        {19.0f, 20.0f, 20.8f, 21.3f, 22.0f},
        {20.0f, 21.3f, 22.1f, 22.7f, 23.5f},
        {21.3f, 22.7f, 23.8f, 24.4f, 25.1f},
        {24.0f, 25.5f, 26.8f, 27.5f, 28.1f},
        {32.4f, 32.9f, 29.3f, 30.5f, 30.9f},
        {36.9f, 37.4f, 37.2f, 37.8f, 35.3f},
        {45.1f, 45.2f, 45.5f, 46.2f, 46.7f},
    },
    {   /* normal steel, Sa */
        {17.5f, 18.3f, 19.1f, 19.8f, 20.6f},
        {17.3f, 18.5f, 19.5f, 20.4f, 21.3f},
        {18.5f, 20.0f, 21.2f, 22.2f, 23.5f},
        {27.0f, 21.0f, 22.4f, 23.7f, 25.2f},
        {28.5f, 28.8f, 29.3f, 29.9f, 28.4f},
        {31.9f, 32.4f, 32.7f, 33.0f, 33.1f},
        {34.5f, 34.9f, 35.4f, 35.8f, 36.2f},
        {39.8f, 40.1f, 40.5f, 41.1f, 41.6f},
        {55.8f, 54.9f, 55.3f, 56.1f, 56.6f},
    },
    {   /* normal steel, preview note Sa+500 */
        {18.0f, 19.4f, 19.9f, 20.2f, 21.1f},
        {25.0f, 24.9f, 25.4f, 26.2f, 27.7f},
        {27.6f, 28.1f, 28.6f, 28.8f, 28.8f},
        {29.9f, 30.6f, 31.2f, 31.3f, 31.6f},
        {32.9f, 33.4f, 34.0f, 34.2f, 34.5f},
        {37.9f, 38.1f, 38.7f, 39.1f, 39.8f},
        {41.5f, 42.3f, 42.6f, 43.2f, 43.6f},
        {49.2f, 49.3f, 49.7f, 50.2f, 50.7f},
        {73.6f, 75.0f, 75.8f, 74.0f, 73.3f},
    },
};
static const float PITCH_UPPER[2][PITCH_SA_N][PITCH_JAWARI_N] = {
    {   /* shortened upper steel, Sa^ */
        {17.9f, 18.9f, 19.6f, 20.2f, 21.0f},
        {18.4f, 19.7f, 20.6f, 21.3f, 22.3f},
        {22.0f, 22.1f, 22.7f, 23.4f, 24.3f},
        {27.9f, 28.3f, 25.5f, 25.5f, 26.1f},
        {30.7f, 31.1f, 30.7f, 31.8f, 33.3f},
        {35.3f, 35.7f, 36.2f, 36.4f, 36.3f},
        {38.3f, 39.1f, 39.4f, 39.9f, 40.2f},
        {45.5f, 45.9f, 46.4f, 47.1f, 47.4f},
        {53.6f, 54.8f, 55.0f, 55.4f, 56.3f},
    },
    {   /* shortened upper steel, Ma^ */
        {25.4f, 26.9f, 27.8f, 28.1f, 28.1f},
        {26.4f, 28.5f, 29.5f, 29.8f, 29.8f},
        {29.2f, 31.4f, 33.3f, 33.7f, 33.7f},
        {38.7f, 33.7f, 35.7f, 36.4f, 36.4f},
        {41.4f, 42.2f, 38.9f, 39.6f, 39.6f},
        {47.7f, 48.6f, 48.9f, 49.6f, 49.6f},
        {51.9f, 52.9f, 53.6f, 53.8f, 53.8f},
        {60.0f, 60.8f, 61.9f, 62.0f, 62.0f},
        {75.6f, 75.3f, 78.5f, 75.6f, 75.6f},
    },
};
static const float PITCH_KHARAJ[PITCH_SA_N][PITCH_JAWARI_N] = {
    {20.8f, 21.6f, 22.2f, 22.6f, 22.9f},
    {21.1f, 22.0f, 22.6f, 23.0f, 23.4f},
    {22.7f, 23.6f, 24.3f, 24.8f, 25.2f},
    {24.4f, 25.4f, 26.1f, 26.7f, 27.1f},
    {25.7f, 26.9f, 27.7f, 28.4f, 28.8f},
    {29.5f, 31.1f, 32.1f, 32.8f, 33.4f},
    {32.4f, 33.8f, 35.1f, 35.9f, 36.5f},
    {36.5f, 38.9f, 40.3f, 41.5f, 42.3f},
    {39.8f, 40.1f, 40.3f, 39.2f, 39.8f},
};

static float sa_jawari_grid(const float *sa_axis, int n_sa, const float table[][PITCH_JAWARI_N],
                            float sa, int jawari)
{
    float jw = (float)jawari * 0.04f, fs, fj;
    int i = 0, j = (int)jw;
    sa = fm_clampf(sa, sa_axis[0], sa_axis[n_sa - 1]);
    while (i < n_sa - 2 && sa > sa_axis[i + 1])
        i++;
    if (j > 3)
        j = 3;
    fs = (sa - sa_axis[i]) / (sa_axis[i + 1] - sa_axis[i]);
    fj = jw - (float)j;
    return (1.0f - fs) * ((1.0f - fj) * table[i][j] + fj * table[i][j + 1])
           + fs * ((1.0f - fj) * table[i + 1][j] + fj * table[i + 1][j + 1]);
}

enum { PITCH_PROFILE_NORMAL, PITCH_PROFILE_UPPER, PITCH_PROFILE_KHARAJ };

static float pitch_compensation(const tanpura_t *m, int profile, float relative_cents)
{
    float sa = (float)(m->tuning.sa_cents + reg_offset(m));
    if (profile == PITCH_PROFILE_KHARAJ)
        return sa_jawari_grid(PITCH_SA, PITCH_SA_N, PITCH_KHARAJ, sa, m->jawari);
    if (profile == PITCH_PROFILE_UPPER) {
        float u = fm_clampf(relative_cents * (1.0f / 500.0f), 0.0f, 1.0f);
        float a = sa_jawari_grid(PITCH_SA, PITCH_SA_N, PITCH_UPPER[0], sa, m->jawari);
        float b = sa_jawari_grid(PITCH_SA, PITCH_SA_N, PITCH_UPPER[1], sa, m->jawari);
        return (1.0f - u) * a + u * b;
    }
    relative_cents = fm_clampf(relative_cents, PITCH_NORMAL_NOTE[0], PITCH_NORMAL_NOTE[PITCH_NORMAL_N - 1]);
    {
        int i = 0;
        float u, a, b;
        while (i < PITCH_NORMAL_N - 2 && relative_cents > PITCH_NORMAL_NOTE[i + 1])
            i++;
        u = (relative_cents - PITCH_NORMAL_NOTE[i]) / (PITCH_NORMAL_NOTE[i + 1] - PITCH_NORMAL_NOTE[i]);
        a = sa_jawari_grid(PITCH_SA, PITCH_SA_N, PITCH_NORMAL[i], sa, m->jawari);
        b = sa_jawari_grid(PITCH_SA, PITCH_SA_N, PITCH_NORMAL[i + 1], sa, m->jawari);
        return (1.0f - u) * a + u * b;
    }
}

/* the compensation gain for the current Sa and jawari: bilinear in the table (main loop, not per sample) */
static float level_comp(const tanpura_t *m)
{
    float sa = (float)(m->tuning.sa_cents + reg_offset(m));
    float db = sa_jawari_grid(LEVEL_SA, 6, LEVEL_DB, sa, m->jawari);
    return fm_exp2f(db * 0.166096405f);          /* 10^(db/20) */
}
#define PLUCK_AMP (-0.4f)          /* N, toward the bridge (the paper's) */
#define PLUCK_TAU 0.05f            /* s */
#define FADE_OUT_S 0.3f
#define PREVIEW_HOLD_S 5.0f
static float cycle_at(const tanpura_t *m, int j)
{
    if (m->extra < 0)
        return j == 0 ? 0.0f : j == 1 ? 0.20f : j == 2 ? 0.35f : j == 3 ? 0.50f : 0.65f;
    return j == 0 ? 0.0f : j == 4 ? 0.20f : j == 1 ? 0.35f : j == 2 ? 0.50f : 0.65f;
}

static int32_t clampi(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }

static void lowpass_coefficients(float c[2][5], int32_t cutoff)
{
    static const float q[2] = {0.5411961f, 1.3065630f};
    float w, cw, sw;
    int k;
    if (cutoff >= TANPURA_FILTER_MAX_HZ) {
        for (k = 0; k < 2; k++) {
            int j;
            for (j = 0; j < 5; j++)
                c[k][j] = UPS_LP[k][j];
        }
        return;
    }
    w = 2.0f * FM_PI * (float)cutoff / TANPURA_FS;
    cw = fm_cosf(w), sw = fm_sinf(w);
    for (k = 0; k < 2; k++) {
        float alpha = sw / (2.0f * q[k]), a0 = 1.0f + alpha;
        c[k][0] = (1.0f - cw) / (2.0f * a0);
        c[k][1] = (1.0f - cw) / a0;
        c[k][2] = c[k][0];
        c[k][3] = -2.0f * cw / a0;
        c[k][4] = (1.0f - alpha) / a0;
    }
}

void tanpura_default_config(tanpura_config_t *cfg)
{
    int j;
    tuning_default(&cfg->tuning);
    cfg->fmax[0] = cfg->fmax[4] = 6000.0f, cfg->fmax[1] = cfg->fmax[2] = 8000.0f, cfg->fmax[3] = 3000.0f;   /* step 2b */
    for (j = 0; j < TANPURA_NSTR; j++)
        cfg->maxm[j] = TSTRING_MAXM;
    cfg->s0 = 0.4f;
    cfg->rc = 0.8f;
    cfg->seed = 0x2545F491u;
    cfg->eq = 0;                                 /* off: ElectroNick heard no difference (step 2b) */
}

/* the register (step 3): the tanpura an octave under the player's Sa (1, "large": the default) when that stays in
 * the range the strings cover (Sa F#2 and up), else at the Sa itself */
static int32_t reg_offset(const tanpura_t *m)
{
    return m->reg && m->tuning.sa_cents - 1200 >= TUNING_SA_MIN ? -1200 : 0;
}

/* the string parameters for string j at the current settings */
static void string_par(const tanpura_t *m, int j, tstring_par_t *p)
{
    int32_t fine = j < TANPURA_NFINE ? m->fine[j] : 0;
    float cents = (float)(fine + reg_offset(m)), relative_cents = (float)fine, r, rho, E, k = 1.0f;
    int profile = PITCH_PROFILE_NORMAL;
    float high_sa = fm_clampf((float)m->tuning.sa_cents * (1.0f / 700.0f), 0.0f, 1.0f);
    if (j == 0 || j == TANPURA_NSTR - 1) {
        int swara = j == 0 ? m->first : m->extra >= 0 ? m->extra : SW_NI;
        float interval = 100.0f * (float)swara - 1200.0f;
        cents += interval;                             /* 12..17: the upper Sa .. Ma, above the player's Sa */
        relative_cents += interval;
        if (swara > SW_NI) {
            k = fm_exp2f(-(float)(100 * swara - 1200) * (1.0f / 1200.0f));
            profile = PITCH_PROFILE_UPPER;
        }
    }
    if (j == 3) {
        cents -= 1200.0f;
        relative_cents -= 1200.0f;
        profile = PITCH_PROFILE_KHARAJ;
    }
    /* the instrument follows Sa (step 2b): the large tanpura (1.1 m, steel 0.16 mm, kharaj bronze 0.25 mm) at Sa A2
     * and below, the smaller one (0.85 m, 0.13 / 0.18 mm) at Sa A3, both heard and liked by ElectroNick at step 2a;
     * in between and above (to 0.7 m at G4) by interpolation in cents, so the tension stays realistic */
    {
        float t = fm_clampf(((float)(m->tuning.sa_cents + reg_offset(m)) + 1500.0f) * (1.0f / 1200.0f), 0.0f, 1.83f);
        p->L = 1.1f - 0.25f * t;
        if (k < 1.0f)                             /* an upper swara note shortens in step with the pitch */
        {                                        /* (the thread moves out by 1/k too: measured, the same jawari and highs as the lower notes; at the old distance the highs were 10 dB up and the jawari short) */
            p->L *= k;
        }
        if (j == 3)
            r = (0.25f - 0.07f * t) * 1e-3f, rho = 8800.0f, E = 1.1e11f;   /* kharaj: bronze */
        else
            r = (0.16f - 0.03f * t) * 1e-3f, rho = 7850.0f, E = 2.0e11f;   /* steel */
    }
    p->rhoA = rho * FM_PI * r * r;
    p->EI = E * 0.25f * FM_PI * r * r * r * r;
    p->f1 = tuning_hz(&m->tuning, cents - pitch_compensation(m, profile, relative_cents));
    p->s0 = m->cfg.s0, p->s1 = 6.5e-3f, p->s3 = 5.0e-6f;     /* Table 1, s0 from step 2a */
    if (k < 1.0f)
        p->s0 = 0.20f;                          /* reduce decay for shortened upper swara strings */
    p->xb = 10e-3f, p->wb = 2.0e-3f, p->kb = 4.39e8f;
    p->xc = (4e-3f + 4e-3f * (float)m->jawari * 0.01f) / k, p->wc = 1.2e-3f, p->kc = 1.2e5f, p->rc = m->cfg.rc;
    if (p->xc > 8.5e-3f) p->xc = 8.5e-3f;          /* (the thread stays short of the bridge's own 10 mm) */
    p->xe = 0.35f * p->L, p->we = 0.02f;
    p->fmax = m->cfg.fmax[j] * (1.0f + high_sa);
    p->maxm = m->cfg.maxm[j];
}

/* prepare string j's coefficients for tanpura_take (main loop). A retune within a semitone keeps the string
 * sounding (tuning by ear while the drone plays); a larger one would change its stiffness, and so the energy its
 * state holds, too much: the render fades out, then tanpura_take silences the strings, takes it, and starts the cycle
 * again */
static void stage(tanpura_t *m, int j)
{
    tstring_par_t p;
    float r;
    string_par(m, j, &p);
    m->stage_ready[j] = 0;                       /* (the render skips a half-written stage) */
    tstring_coefs(&m->stage[j], &p, TANPURA_FS);
    r = p.f1 / (m->f1_now[j] > 0.0f ? m->f1_now[j] : p.f1);
    if (r > 1.0595f || r < 0.9439f)              /* more than a semitone */
        m->restart = 1;
    m->f1_now[j] = p.f1;
    m->comp_target = level_comp(m);
    m->stage_ready[j] = 1;
}

static uint32_t rnd(tanpura_t *m)                /* xorshift32 */
{
    uint32_t x = m->rng;
    x ^= x << 13, x ^= x >> 17, x ^= x << 5;
    return m->rng = x;
}
static float rnd_pm1(tanpura_t *m) { return (float)(int32_t)(rnd(m) >> 8) * (1.0f / 8388608.0f) - 1.0f; }

static int32_t jitter(tanpura_t *m)
{
    return (int32_t)(rnd_pm1(m) * (float)m->human * 0.01f * 0.05f * TANPURA_FS);
}

static int32_t cycle_samples(const tanpura_t *m) { return (int32_t)((float)m->cycle * 0.1f * TANPURA_FS); }

static void start_cycle(tanpura_t *m)
{
    int j;
    m->phase_done = m->phase;
    for (j = 0; j < TANPURA_NSTR; j++) {
        m->nominal[j] = (int32_t)(cycle_at(m, j) * (float)cycle_samples(m)) + (int32_t)(0.02f * TANPURA_FS)
                        + cycle_samples(m) / 100 * m->phase;
        m->due[j] = m->nominal[j] + (j == TANPURA_NSTR - 1 && m->extra < 0 ? 0 : jitter(m));
    }
}

void tanpura_init(tanpura_t *m, const tanpura_config_t *cfg)
{
    int j, k;
    if (cfg)
        m->cfg = *cfg;
    else
        tanpura_default_config(&m->cfg);
    m->tuning = m->cfg.tuning;
    m->on = 0, m->first = SW_PA, m->extra = -1, m->jawari = 50, m->cycle = 50, m->human = 50, m->reg = 1;
    m->filter_hz = TANPURA_FILTER_MAX_HZ, m->lp_stage_ready = 0;
    lowpass_coefficients(m->lp_coef, m->filter_hz);
    m->start_req = m->extra_silence_req = 0;
    m->preview_ready = m->preview_cancel = 0;
    m->preview_left = m->preview_active = m->preview_cents_above_c4 = 0;
    m->phase = m->phase_done = 0;
    for (j = 0; j < TANPURA_NFINE; j++)
        m->fine[j] = 0;
    for (j = 0; j < TANPURA_NSTR; j++)
        m->vol[j] = 100;
    m->rng = m->cfg.seed ? m->cfg.seed : 1u;
    for (j = 0; j < TANPURA_NSTR; j++)
        m->f1_now[j] = 0.0f;
    for (j = 0; j < TANPURA_NSTR; j++) {
        stage(m, j);
        tstring_init(&m->str[j], &m->stage[j]);
        m->stage_ready[j] = 0;
    }
    tstring_init(&m->preview, &m->stage[1]);
    for (k = 0; k < 32; k++)
        m->bw1[k] = m->bw2[k] = 0.0f;
    m->xd1 = m->xn1 = 0.0f;
    for (k = 0; k < (int)(sizeof m->up / sizeof m->up[0]); k++)
        m->up[k] = 0.0f;
    m->lp[0][0] = m->lp[0][1] = m->lp[1][0] = m->lp[1][1] = 0.0f;
    m->eqs[0] = m->eqs[1] = 0.0f;
    m->gain = 0.0f;
    m->active = 0;
    m->restart = 0;
    m->comp = m->comp_target;
    start_cycle(m);
}

void tanpura_set_param(tanpura_t *m, int id, int32_t v)
{
    int j;
    if (tuning_set_param(&m->tuning, id, v)) {
        for (j = 0; j < TANPURA_NSTR; j++)
            stage(m, j);
        return;
    }
    if (id >= TANPURA_P_FINE && id < TANPURA_P_FINE + TANPURA_NFINE) {
        m->fine[id - TANPURA_P_FINE] = clampi(v, -50, 50);
        stage(m, id - TANPURA_P_FINE);
        return;
    }
    if (id >= TANPURA_P_VOL && id < TANPURA_P_VOL + TANPURA_NSTR) {
        m->vol[id - TANPURA_P_VOL] = clampi(v, 0, 100);
        return;
    }
    switch (id) {
    case TANPURA_P_ON:
        v = v != 0;
        if (v && !m->on)
            m->start_req = 1;                    /* (tanpura_take starts the cycle: the render may be running) */
        m->preview_cancel = 1;
        m->preview_ready = 0;
        m->on = v;
        break;
    case TANPURA_P_REGISTER:
        m->reg = v != 0;
        for (j = 0; j < TANPURA_NSTR; j++)
            stage(m, j);
        break;
    case TANPURA_P_FIRST:  m->first = clampi(v, 0, TANPURA_FIRST_MAX); stage(m, 0); break;
    case TANPURA_P_JAWARI:
        m->jawari = clampi(v, 0, 100);
        for (j = 0; j < TANPURA_NSTR; j++)
            stage(m, j);
        break;
    case TANPURA_P_CYCLE:  m->cycle = clampi(v, 25, 80); break;
    case TANPURA_P_HUMAN:  m->human = clampi(v, 0, 100); break;
    case TANPURA_P_PHASE:  m->phase = clampi(v, 0, 100); break;
    case TANPURA_P_EXTRA:
        v = clampi(v, -1, TANPURA_FIRST_MAX);
        if (v != m->extra) {
            m->extra = v;
            stage(m, TANPURA_NSTR - 1);
            m->extra_silence_req = v < 0;
            m->start_req = 1;
        }
        break;
    case TANPURA_P_FILTER:
        v = clampi(v, TANPURA_FILTER_MIN_HZ, TANPURA_FILTER_MAX_HZ);
        if (v != m->filter_hz) {
            m->filter_hz = v;
            lowpass_coefficients(m->lp_stage, v);
            m->lp_stage_ready = 1;
        }
        break;
    default: break;
    }
}

void tanpura_preview(tanpura_t *m, int32_t cents_above_c4)
{
    tstring_par_t p;
    float relative_cents = (float)(cents_above_c4 - m->tuning.sa_cents - reg_offset(m));
    m->preview_cents_above_c4 = cents_above_c4;
    m->preview_cancel = 0;
    m->preview_ready = 0;
    string_par(m, 1, &p);
    p.f1 = tuning_hz(&m->tuning, (float)cents_above_c4 - (float)m->tuning.sa_cents
                     - pitch_compensation(m, PITCH_PROFILE_NORMAL, relative_cents));
    tstring_coefs(&m->preview_stage, &p, TANPURA_FS);
    m->preview_ready = 1;
}

int32_t tanpura_get_param(const tanpura_t *m, int id)
{
    if (id == DSP_P_A4_DHZ) return m->tuning.a4_dhz;
    if (id == DSP_P_SA_CENTS) return m->tuning.sa_cents;
    if (id >= TANPURA_P_FINE && id < TANPURA_P_FINE + TANPURA_NFINE) return m->fine[id - TANPURA_P_FINE];
    if (id >= TANPURA_P_VOL && id < TANPURA_P_VOL + TANPURA_NSTR) return m->vol[id - TANPURA_P_VOL];
    switch (id) {
    case TANPURA_P_ON:     return m->on;
    case TANPURA_P_FIRST:  return m->first;
    case TANPURA_P_JAWARI: return m->jawari;
    case TANPURA_P_CYCLE:  return m->cycle;
    case TANPURA_P_HUMAN:  return m->human;
    case TANPURA_P_REGISTER: return m->reg;
    case TANPURA_P_SA_NOW: return m->tuning.sa_cents + reg_offset(m);
    case TANPURA_P_PHASE:  return m->phase;
    case TANPURA_P_EXTRA:  return m->extra;
    case TANPURA_P_FILTER: return m->filter_hz;
    default:               return 0;
    }
}

void tanpura_take(tanpura_t *m)
{
    int j;
    if (m->lp_stage_ready) {
        int k, i;
        for (k = 0; k < 2; k++)
            for (i = 0; i < 5; i++)
                m->lp_coef[k][i] = m->lp_stage[k][i];
        m->lp_stage_ready = 0;
    }
    if (m->restart && (m->gain == 0.0f || !m->active)) {   /* a large retune, faded out (or silent): afresh */
        for (j = 0; j < TANPURA_NSTR; j++)
            tstring_silence(&m->str[j]);
        start_cycle(m);
        m->restart = 0;
    }
    if (!m->restart)
        for (j = 0; j < TANPURA_NSTR; j++)
            if (m->stage_ready[j]) {
                tstring_take(&m->str[j], &m->stage[j]);
                m->stage_ready[j] = 0;
            }
    if (m->extra_silence_req) {
        tstring_silence(&m->str[TANPURA_NSTR - 1]);
        m->extra_silence_req = 0;
    }
    if (m->phase != m->phase_done) {             /* a running cycle follows the new offset */
        int32_t d = cycle_samples(m) / 100 * (m->phase - m->phase_done);
        for (j = 0; j < TANPURA_NSTR; j++)
            m->nominal[j] += d, m->due[j] += d;
        m->phase_done = m->phase;
    }
    if (m->start_req) {
        start_cycle(m);
        m->active = 1;
        m->start_req = 0;
    }
    if (m->preview_cancel) {
        tstring_silence(&m->preview);
        m->preview_active = 0;
        m->preview_left = 0;
        m->preview_cancel = 0;
    }
    if (m->preview_ready && !m->restart) {
        tstring_init(&m->preview, &m->preview_stage);
        tstring_pluck(&m->preview, PLUCK_AMP * (float)m->vol[1] * 0.01f, PLUCK_TAU, TANPURA_FS);
        m->preview_left = (int32_t)(PREVIEW_HOLD_S * DSP_FS);
        m->preview_active = 1;
        m->active = 1;
        m->preview_ready = 0;
    }
}

/* string j for n samples from offset `at` of the block, its plucks at their sample */
static void run_string(tanpura_t *m, int j, int n)
{
    tstring_t *s = &m->str[j];
    int at = 0;
    while (at < n) {
        int len = n - at;
        if (m->on && m->due[j] < len)
            len = m->due[j] < 0 ? 0 : m->due[j];
        if (len > 0) {
            tstring_run(s, m->fd + at, m->fn + at, len);
            at += len;
            m->due[j] -= len;
            m->nominal[j] -= len;
        }
        if (m->on && m->due[j] <= 0 && at < n) {
            float a = PLUCK_AMP * (float)m->vol[j] * 0.01f * (1.0f + 0.003f * (float)m->human * rnd_pm1(m));
            tstring_pluck(s, a, PLUCK_TAU, TANPURA_FS);
            m->nominal[j] += cycle_samples(m);
            m->due[j] = m->nominal[j] + jitter(m);
        }
    }
}

void tanpura_render(tanpura_t *m, int32_t *out, int n)
{
    float g0 = m->gain, g1, dg, *x = m->up + 2 * UPS_K;   /* x[-2K..-1]: the history, x[0..ns-1]: this block */
    float c0 = m->comp, c1 = m->comp_target, dc;
    int i, j, k, ns = n / TANPURA_OS;
    dc = (c1 - c0) / (float)ns;
    if (!m->active)
        return;
    for (i = 0; i < ns; i++)
        m->fd[i] = m->fn[i] = 0.0f;
    for (j = 0; j < TANPURA_NSTR; j++)
        if (j != TANPURA_NSTR - 1 || m->extra >= 0)
            run_string(m, j, ns);
    if (m->preview_active)
        tstring_run(&m->preview, m->fd, m->fn, ns);
    /* the body at the strings' rate: parallel sections with shared poles, the bridge and nut forces in */
    for (i = 0; i < ns; i++) {
        float xd = m->fd[i], xn = m->fn[i], p = BODY_D_BRIDGE * xd + BODY_D_NUT * xn;
        for (k = 0; k < BODY_NSEC; k++) {
            float w = BODY_B0_BRIDGE[k] * xd + BODY_B1_BRIDGE[k] * m->xd1 + BODY_B0_NUT[k] * xn
                      + BODY_B1_NUT[k] * m->xn1 - BODY_A1[k] * m->bw1[k] - BODY_A2[k] * m->bw2[k];
            m->bw2[k] = m->bw1[k];
            m->bw1[k] = w;
            p += w;
        }
        m->xd1 = xd, m->xn1 = xn;
        if (m->cfg.eq) {                         /* the brightness correction (upsample_coefs.h) */
            float yv = UPS_EQ[0] * p + m->eqs[0];
            m->eqs[0] = UPS_EQ[1] * p - UPS_EQ[3] * yv + m->eqs[1];
            m->eqs[1] = UPS_EQ[2] * p - UPS_EQ[4] * yv;
            p = yv;
        }
        for (k = 0; k < UPS_LP_N; k++) {
            float yv = m->lp_coef[k][0] * p + m->lp[k][0];
            m->lp[k][0] = m->lp_coef[k][1] * p - m->lp_coef[k][3] * yv + m->lp[k][1];
            m->lp[k][1] = m->lp_coef[k][2] * p - m->lp_coef[k][4] * yv;
            p = yv;
        }
        x[i] = p * TANPURA_LEVEL * (c0 + dc * (float)(i + 1));
    }
    m->comp = c1;
    /* up to the FM-1's rate (upsample_coefs.h), with the on/off fade */
    if (m->restart)                              /* a large retune: out in 20 ms */
        g1 = g0 - (float)n / (0.02f * DSP_FS) < 0.0f ? 0.0f : g0 - (float)n / (0.02f * DSP_FS);
    else if (m->on || (m->preview_active && m->preview_left > 0))
        g1 = g0 + (float)n / (0.02f * DSP_FS) > 1.0f ? 1.0f : g0 + (float)n / (0.02f * DSP_FS);
    else
        g1 = g0 - (float)n / (FADE_OUT_S * DSP_FS) < 0.0f ? 0.0f : g0 - (float)n / (FADE_OUT_S * DSP_FS);
    dg = (g1 - g0) / (float)n;
    for (i = 0; i < ns; i++) {
        const float *c = x + i - UPS_K;           /* the centre: out[2i] = c[0] */
        float odd = 0.0f, e = c[0], o;
        for (k = 0; k < UPS_K; k++)
            odd += UPS_H[k] * (c[-k] + c[1 + k]);
        o = odd;
        e = fm_clampf(e * (g0 + dg * (float)(2 * i + 1)), -1.0f, 1.0f);
        o = fm_clampf(o * (g0 + dg * (float)(2 * i + 2)), -1.0f, 1.0f);
        out[2 * i] += (int32_t)(e * DSP_Q23_F);
        out[2 * i + 1] += (int32_t)(o * DSP_Q23_F);
    }
    for (k = 0; k < 2 * UPS_K; k++)              /* keep the last 2K inputs as the next block's history */
        m->up[k] = x[ns - 2 * UPS_K + k];
    m->gain = g1;
    if (m->preview_active && m->preview_left > 0)
        m->preview_left = m->preview_left > n ? m->preview_left - n : 0;
    if (!m->on && g1 == 0.0f) {                  /* faded out: silence and stop computing */
        for (j = 0; j < TANPURA_NSTR; j++)
            tstring_silence(&m->str[j]);
        tstring_silence(&m->preview);
        m->preview_active = 0;
        m->preview_left = 0;
        for (k = 0; k < 32; k++)
            m->bw1[k] = m->bw2[k] = 0.0f;
        m->xd1 = m->xn1 = 0.0f;
        for (k = 0; k < (int)(sizeof m->up / sizeof m->up[0]); k++)
            m->up[k] = 0.0f;
        m->lp[0][0] = m->lp[0][1] = m->lp[1][0] = m->lp[1][1] = 0.0f;
        m->eqs[0] = m->eqs[1] = 0.0f;
        m->active = 0;
    }
}
