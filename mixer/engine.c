/* engine.c — see engine.h */
#include "mixer/engine.h"
#include "dsp/sine.h"

void engine_init(engine_t *e)
{
    sine_init();
    tanpura_config_t cfg;
    tanpura_init(&e->tanpura, 0);
    tanpura_attach_preview(&e->tanpura, &e->preview);
    tanpura_default_config(&cfg);
    cfg.seed ^= 0x9E3779B9u;                     /* its own irregularity */
    tanpura_init(&e->tanpura2, &cfg);
    tanpura_set_param(&e->tanpura2, TANPURA_P_FIRST, ENGINE_TAN2_FIRST);
    tanpura_set_param(&e->tanpura2, TANPURA_P_PHASE, ENGINE_TAN2_PHASE);
    e->tan_on = 0, e->tan2_on = 0;
    tabla_init(&e->tabla, 0);
    taal_init(&e->taal, 0);
    reftone_init(&e->reftone, 0);
    mixer_init(&e->mixer);
    e->muted = 0;
}

void engine_set_param(engine_t *e, int target, int id, int32_t v)
{
    if (target == ENG_ALL || target == ENG_TANPURA) {
        tanpura_set_param(&e->tanpura, id, v);
        if (id == TANPURA_P_ON) {
            e->tan_on = v != 0;
        } else if (id != TANPURA_P_FIRST && id != TANPURA_P_PHASE && id != TANPURA_P_EXTRA && id != TANPURA_P_MUTE)
            tanpura_set_param(&e->tanpura2, id, v);
    }
    if (target == ENG_TANPURA2) {
        if (id == TANPURA_P_ON) {
            e->tan2_on = v != 0;
            tanpura_set_param(&e->tanpura2, id, e->tan2_on);
        } else if (id == TANPURA_P_FIRST || id == TANPURA_P_PHASE || id == TANPURA_P_EXTRA || id == TANPURA_P_MUTE)
            tanpura_set_param(&e->tanpura2, id, v);
    }
    if (target == ENG_ALL || target == ENG_TABLA)
        tabla_set_param(&e->tabla, id, v);
    if (target == ENG_ALL || target == ENG_REFTONE)
        reftone_set_param(&e->reftone, id, v);
    if (target == ENG_MIXER)
        mixer_set_param(&e->mixer, id, v);
    if (target == ENG_TAAL)
        taal_set_param(&e->taal, id, v);
}

int32_t engine_get_param(const engine_t *e, int target, int id)
{
    if (target == ENG_MIXER)
        return id >= MIX_P_LEVEL && id < MIX_P_LEVEL + MIX_NSRC ? e->mixer.level[id - MIX_P_LEVEL] : 0;
    if (target == ENG_TANPURA)
        return tanpura_get_param(&e->tanpura, id);
    if (target == ENG_TANPURA2)
        return id == TANPURA_P_ON ? e->tan2_on : tanpura_get_param(&e->tanpura2, id);
    if (target == ENG_TABLA)
        return tabla_get_param(&e->tabla, id);
    if (target == ENG_TAAL)
        return taal_get_param(&e->taal, id);
    return reftone_get_param(&e->reftone, id);   /* the shared tuning (the same in every module) and the tone */
}

void engine_tanpura_take(engine_t *e)
{
    tanpura_take(&e->tanpura);
    tanpura_take(&e->tanpura2);
}

void engine_tanpura_preview(engine_t *e, int32_t cents_above_c4)
{
    if (!e->tan_on && !e->tan2_on)
        tanpura_preview(&e->tanpura, cents_above_c4);
}

void engine_tanpura_render(engine_t *e, int32_t *tan, uint32_t n)
{
    uint32_t k;
    for (k = 0; k < n; k++)
        tan[k] = 0;
    for (k = 0; k + DSP_BLOCK <= n; k += DSP_BLOCK)
        tanpura_render(&e->tanpura, tan + k, DSP_BLOCK);
}

void engine_tanpura2_render(engine_t *e, int32_t *tan2, uint32_t n)
{
    uint32_t k;
    for (k = 0; k < n; k++)
        tan2[k] = 0;
    for (k = 0; k + DSP_BLOCK <= n; k += DSP_BLOCK)
        tanpura_render(&e->tanpura2, tan2 + k, DSP_BLOCK);
}

void engine_render_with(engine_t *e, int32_t *lr, uint32_t n, uint32_t master_q12, const int32_t *tan, const int32_t *tan2)
{
    int32_t *const src[MIX_NSRC] = {e->buf[MIX_TANPURA], e->buf[MIX_TABLA], e->buf[MIX_REFTONE]};
    uint32_t k;
    int i, s;
    if (e->muted) {
        for (k = 0; k < 2u * n; k++)
            lr[k] = 0;
        return;
    }
    for (k = 0; k + DSP_BLOCK <= n; k += DSP_BLOCK) {
        for (s = 0; s < MIX_NSRC; s++)
            for (i = 0; i < DSP_BLOCK; i++)
                e->buf[s][i] = 0;
        for (i = 0; i < DSP_BLOCK; i++)
            e->buf[MIX_TANPURA][i] = (tan[k + (uint32_t)i] + tan2[k + (uint32_t)i]) * 3 / 4;
        {
            taal_event_t ev[16];
            int ne = taal_advance(&e->taal, DSP_BLOCK, ev, 16), j;
            for (j = 0; j < ne; j++)
                tabla_trigger(&e->tabla, ev[j].stroke, ev[j].dynamic, ev[j].offset);
        }
        tabla_render(&e->tabla, e->buf[MIX_TABLA], DSP_BLOCK);
        reftone_render(&e->reftone, e->buf[MIX_REFTONE], DSP_BLOCK);
        mixer_render(&e->mixer, src, lr + 2u * k, DSP_BLOCK, master_q12);
    }
    for (; k < n; k++)             /* (n not a multiple of DSP_BLOCK: the rest silent; never on the FM-1) */
        lr[2u * k] = lr[2u * k + 1] = 0;
}

void engine_render(engine_t *e, int32_t *lr, uint32_t n, uint32_t master_q12)
{
    if (e->muted) {
        engine_render_with(e, lr, n, master_q12, e->tan, e->tan2);
        return;
    }
    engine_tanpura_take(e);
    engine_tanpura_render(e, e->tan, n);
    engine_tanpura2_render(e, e->tan2, n);
    engine_render_with(e, lr, n, master_q12, e->tan, e->tan2);
}

void engine_panic(engine_t *e)
{
    e->muted = 1;
}
