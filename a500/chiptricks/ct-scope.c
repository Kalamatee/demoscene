#include "ahx.h"
#include "blitter.h"
#include "memory.h"
#include "ilbm.h"
#include "chiptricks.h"
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
static UBYTE *multab;

void WaveScopeInit(char *image) {
  WORD i, j;

  multab = MemAlloc(256*64, MEMF_PUBLIC);
  spans = LoadILBMCustom(image, BM_DISPLAYABLE);

  for (i = 0; i < 64; i++) {
    for (j = 0; j < 128; j++) {
      WORD index = (i << 8) | j;
      WORD x = ((WORD)j * (WORD)(i + 1)) >> 6;
      multab[index] = x & ~3;
    }

    for (; j < 255; j++) {
      WORD index = (i << 8) | j;
      WORD x = ((WORD)(255 - j) * (WORD)(i + 1)) >> 6;
      multab[index] = x & ~3;
    }
  }
}

void WaveScopeKill(void) {
  MemFree(multab);
  DeleteBitmap(spans);
}

void WaveScopeUpdateChannel(WORD num) {
  AhxVoiceTempT *voice = &Ahx.Public->VoiceTemp[num];
  WaveScopeChanT *ch = &channel[num];
  WORD samplesPerFrame = 1;

  if (voice->AudioPeriod)
    samplesPerFrame = div16(TICKS_PER_FRAME, voice->AudioPeriod);

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
  WORD dststart = ((x & ~15) >> 3) + (WORD)y * (WORD)dst->bytesPerRow;
  UBYTE *samples = ch->samples;
  LONG i = ch->i;
  LONG di = ch->di;
  WORD n;

  BitmapT *src = spans;
  APTR srcbpt = src->planes[0];
  APTR dstbpt = dst->planes[0];
  UBYTE *_multab = multab + (ch->volume << 8);
  UWORD bltsize = (src->depth << 6) | (src->bytesPerRow >> 1);

  WaitBlitter();

  custom->bltalwm = -1;
  custom->bltafwm = -1;
  custom->bltamod = src->bplSize - src->bytesPerRow;
  custom->bltdmod = dst->bplSize - src->bytesPerRow;
  custom->bltcon0 = (SRCA | DEST | A_TO_D);
  custom->bltcon1 = 0;

  for (n = 0; n < WS_SAMPLES; n++) {
    WORD srcstart;

    srcstart = _multab[samples[i >> 16]];

    i += di;
    if (i >= (AHX_SAMPLE_LEN << 16))
      i -= (AHX_SAMPLE_LEN << 16);

    {
      APTR _srcbpt = srcbpt + srcstart;
      APTR _dstbpt = dstbpt + dststart;

      WaitBlitter();

      custom->bltapt = _srcbpt;
      custom->bltdpt = _dstbpt;
      custom->bltsize = bltsize;
    }

    dststart += WIDTH / 8; // dst->bytesPerRow;
  }

  ch->i = i;
}
