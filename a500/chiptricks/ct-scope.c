#include "ahx.h"
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

static WaveScopeChanT channel[4];
static BitmapT *spans;
static UBYTE multab[256*64];

void WaveScopeInit(char *image) {
  WORD i, j;

  for (i = 0; i < 64; i++) {
    for (j = 0; j < 128; j++) {
      WORD index = (i << 8) | j;
      WORD x = ((WORD)j * (WORD)(i + 1)) >> 4;
      multab[index] = x;
    }

    for (; j < 255; j++) {
      WORD index = (i << 8) | j;
      WORD x = ((WORD)(255 - j) * (WORD)(i + 1)) >> 4;
      multab[index] = x;
    }
  }

  spans = LoadILBMCustom(image, BM_DISPLAYABLE);
}

void WaveScopeKill(void) {
  DeleteBitmap(spans);
}

void WaveScopeUpdateChannel(WORD num) {
  AhxVoiceTempT *voice = &Ahx.Public->VoiceTemp[num];
  WaveScopeChanT *ch = &channel[num];
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

void WaveScopeDrawChannel(WORD num, BitmapT *dst, WORD x, WORD y) {
  WaveScopeChanT *ch = &channel[num];
  LONG dststart = ((x & ~15) >> 3) + (WORD)y * (WORD)dst->bytesPerRow;
  UBYTE *samples = ch->samples;
  LONG volume = ch->volume << 8;
  LONG i = ch->i;
  LONG di = ch->di;
  WORD n;

  BitmapT *src = spans;
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
      LONG srcstart = x << 2; // (WORD)x * (WORD)src->bytesPerRow;
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
