#ifndef __CT_SCOPE_H__
#define __CT_SCOPE_H__

#include "common.h"
#include "gfx.h"

void WaveScopeInit(char *spans);
void WaveScopeKill(void);
void WaveScopeUpdateChannel(WORD num);
void WaveScopeDrawChannel(WORD num, BitmapT *bm, WORD x, WORD y);

#endif
