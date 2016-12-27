#ifndef __CT_SCOPE_H__
#define __CT_SCOPE_H__

#include "common.h"
#include "gfx.h"
#include "ahx.h"

typedef struct WaveScope WaveScopeT;

WaveScopeT *NewWaveScope(char *spans);
void DeleteWaveScope(WaveScopeT *ws);
void WaveScopeUpdateChannel(WaveScopeT *ws, WORD num, AhxVoiceTempT *voice);
void WaveScopeDrawChannel(WaveScopeT *ws, WORD num,
                          BitmapT *bm, WORD x, WORD y);

#endif
