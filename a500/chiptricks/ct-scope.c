#include "blitter.h"
#include "memory.h"
#include "ilbm.h"
#include "ct-scope.h"

typedef struct {
  BYTE *samples;
  WORD volume;
  LONG i, di;
} WaveScopeChanT;

#define TICKS_PER_FRAME (3579546L / 50)
#define WS_SAMPLES 32

struct WaveScope {
  WaveScopeChanT channel[4];
  BitmapT *spans;
  UBYTE multab[256*64];
};

WaveScopeT *NewWaveScope(char *spans) {
  WaveScopeT *ws = MemAlloc(sizeof(WaveScopeT), MEMF_PUBLIC);
  WORD i, j;

  for (i = 0; i < 4; i++)
    memset(&ws->channel[i], 0, sizeof(WaveScopeChanT));

  for (i = 0; i < 64; i++) {
    for (j = 0; j < 128; j++) {
      WORD index = (i << 8) | j;
      WORD x = (j * (i + 1)) >> 4;
      ws->multab[index] = x;
    }

    for (; j < 255; j++) {
      WORD index = (i << 8) | j;
      WORD x = ((255 - j) * (i + 1)) >> 4;
      ws->multab[index] = x;
    }
  }

  ws->spans = LoadILBMCustom(spans, BM_DISPLAYABLE);

  return ws;
}

void DeleteWaveScope(WaveScopeT *ws) {
  DeleteBitmap(ws->spans);
  MemFree(ws);
}

void WaveScopeUpdateChannel(WaveScopeT *ws, WORD num, AhxVoiceTempT *voice) {
  WaveScopeChanT *ch = &ws->channel[num];
  LONG samplesPerFrame = 1;

  if (voice->AudioPeriod)
    samplesPerFrame = TICKS_PER_FRAME / voice->AudioPeriod;

  ch->samples = voice->AudioPointer;
  ch->volume = voice->AudioVolume;
  ch->di = (samplesPerFrame << 16) / WS_SAMPLES;

  ch->volume -= 1;
  if (ch->volume < 0)
    ch->volume = 0;

  if (ch->samples != voice->AudioPointer)
    ch->i = 0;
}

void WaveScopeDrawChannel(WaveScopeT *ws, WORD num,
                          BitmapT *dst, WORD x, WORD y) 
{
  WaveScopeChanT *ch = &ws->channel[num];
  LONG dststart = ((x & ~15) >> 3) + (WORD)y * (WORD)dst->bytesPerRow;
  UBYTE *samples = ch->samples;
  UBYTE *multab = ws->multab;
  LONG volume = ch->volume << 8;
  LONG i = ch->i;
  LONG di = ch->di;
  WORD n;

  BitmapT *src = ws->spans;
  UWORD bltsize = (src->depth << 6) | (src->bytesPerRow >> 1);

  WaitBlitter();

  custom->bltalwm = -1;
  custom->bltafwm = -1;
  custom->bltamod = src->bplSize - src->bytesPerRow;
  custom->bltdmod = dst->bplSize - src->bytesPerRow;
  custom->bltcon0 = (SRCA | DEST | A_TO_D);
  custom->bltcon1 = 0;

  for (n = 0; n < WS_SAMPLES; n++) {
    LONG x;

    i = swap16(i);
    x = multab[samples[i] | volume] >> 4;
    i = swap16(i);

    i += di;
    if (i >= (AHX_SAMPLE_LEN << 16))
      i -= (AHX_SAMPLE_LEN << 16);

    {
      LONG srcstart = (WORD)x * (WORD)src->bytesPerRow;
      APTR srcbpt = src->planes[0] + srcstart;
      APTR dstbpt = dst->planes[0] + dststart;

      WaitBlitter();

      custom->bltapt = srcbpt;
      custom->bltdpt = dstbpt;
      custom->bltsize = bltsize;
    }

    dststart += dst->bytesPerRow;
  }

  ch->i = i;
}
