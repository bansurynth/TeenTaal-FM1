/* Internal string-parameter construction, also used by the host pitch calibration. */
#pragma once

#include "tanpura/tanpura.h"

void tanpura_string_par(const tanpura_t *m, int string, tstring_par_t *p);
void tanpura_preview_par(const tanpura_t *m, int32_t cents_above_c4, tstring_par_t *p);
