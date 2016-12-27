#include "startup.h"
#include "hardware.h"
#include "interrupts.h"
#include "event.h"
#include "memory.h"
#include "sprite.h"
#include "ilbm.h"
#include "io.h"
#include "keyboard.h"
#include "mouse.h"
#include "coplist.h"
#include "ahx.h"
#include "fx.h"
#include "blitter.h"
#include "color.h"
#include "ct-gui.h"
#include "ct-scope.h"
#include "ct-load.h"

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 5

#define SCOPE_X 224
#define SCOPE_Y 8

#define COLOR 8
#define LABEL_Y 34
#define SCROLL_Y 241
#define SCROLL_H 15

STRPTR __cwdpath = "chiptrick";
LONG __chipmem = 250 * 1024;
LONG __fastmem = 450 * 1024;

static BitmapT *screen, *scroll, *trkselbt[3];
static FontT *small_font, *big_font;
static SpriteT *pointer;
static CopInsT *sprptr[8];
static CopListT *cp;
static WaveScopeT *wavescope;

static GUI_RADIOBT(_b00, 5,  36, 7, 7);
static GUI_RADIOBT(_b01, 5,  46, 7, 7);
static GUI_RADIOBT(_b02, 5,  56, 7, 7);
static GUI_RADIOBT(_b03, 5,  66, 7, 7);
static GUI_RADIOBT(_b04, 5,  76, 7, 7);
static GUI_RADIOBT(_b05, 5,  86, 7, 7);
static GUI_RADIOBT(_b06, 5,  96, 7, 7);
static GUI_RADIOBT(_b07, 5, 106, 7, 7);
static GUI_RADIOBT(_b08, 5, 116, 7, 7);
static GUI_RADIOBT(_b09, 5, 126, 7, 7);
static GUI_RADIOBT(_b10, 5, 136, 7, 7);
static GUI_RADIOBT(_b11, 5, 146, 7, 7);

static GUI_GROUP(_track_bt,
                 _b00, _b01, _b02, _b03, _b04, _b05, 
                 _b06, _b07, _b08, _b09, _b10, _b11);

static GUI_LABEL(_l00, 16,  35, 96, 10);
static GUI_LABEL(_l01, 16,  45, 96, 10);
static GUI_LABEL(_l02, 16,  55, 96, 10);
static GUI_LABEL(_l03, 16,  65, 96, 10);
static GUI_LABEL(_l04, 16,  75, 96, 10);
static GUI_LABEL(_l05, 16,  85, 96, 10);
static GUI_LABEL(_l06, 16,  95, 96, 10);
static GUI_LABEL(_l07, 16, 105, 96, 10);
static GUI_LABEL(_l08, 16, 115, 96, 10);
static GUI_LABEL(_l09, 16, 125, 96, 10);
static GUI_LABEL(_l10, 16, 135, 96, 10);
static GUI_LABEL(_l11, 16, 145, 96, 10);

static GUI_GROUP(_track_lb,
                 _l00, _l01, _l02, _l03, _l04, _l05, 
                 _l06, _l07, _l08, _l09, _l10, _l11);

static GUI_LABEL(_info, 208, 83, 80, 80);
 
static GUI_GROUP(_root, _track_bt, _track_lb, _info);
static GUI_MAIN(_root);

#define MODULES 12

static ArtworkT *artwork;

static struct {
  APTR data;
  CopInsT *bgcol;
} module[MODULES];

static WORD active = 0;

static void SwitchModule(WORD new) {
  AhxStopSong();

  CopInsSet16(module[active].bgcol, (active & 1) ? 0x311 : 0x411);

  active = new;

  if (AhxInitModule(module[active].data) == 0)
    AhxInitSubSong(0, 0);

  // BitmapClear(scroll, 3);
  DrawTextSetup(scroll, 0, big_font);
  DrawText(MAKE(Area2D, 0, 1, WIDTH, 13), artwork->info[active].note);

  _info->label.text = artwork->info[active].author;
  DrawTextSetup(gui->screen, 0, gui->font);
  GuiWidgetRedraw(gui, _info);
}

static void AhxSetTempo(UWORD tempo asm("d0"));

static void Load() {
  if (AhxInitPlayer(AHX_LOAD_WAVES_FILE, AHX_FILTERS))
    exit(10);

  artwork = LoadArtwork("ct-modules.dat");
  screen = LoadILBMCustom("ct_panel.ilbm", BM_DISPLAYABLE|BM_LOAD_PALETTE);
  trkselbt[0] = LoadILBMCustom("ct_track_inactive.ilbm", BM_DISPLAYABLE);
  trkselbt[1] = LoadILBMCustom("ct_track_hover.ilbm", BM_DISPLAYABLE);
  trkselbt[2] = LoadILBMCustom("ct_track_active.ilbm", BM_DISPLAYABLE);
  small_font = LoadFont("ct-small.font");
  big_font = LoadFont("ct-big.font");

  scroll = NewBitmapCustom(WIDTH, HEIGHT - SCROLL_Y + 1, 3,
                           BM_DISPLAYABLE|BM_CLEAR);

  {
    WORD i;

    for (i = 0; i < MODULES; i++) {
      module[i].data = LoadFile(artwork->info[i].filename, MEMF_PUBLIC);
      BUTTON(GROUP(_track_bt)->widgets[i])->images = trkselbt;
      LABEL(GROUP(_track_lb)->widgets[i])->text = artwork->info[i].title;
      // LABEL(GROUP(_track_lb)->widgets[i])->length = strlen(artwork->info[i].title);
    }
  }

  {
    BitmapT *bm = LoadILBMCustom("ct_pointer.ilbm", 0);
    pointer = NewSpriteFromBitmap(16, bm, 0, 0);
    DeleteBitmap(bm);
  }

  wavescope = NewWaveScope("ct_spans.ilbm");
}

static void UnLoad() {
  {
    WORD i;

    for (i = 0; i < MODULES; i++)
      MemFree(module[i].data);
  }

  DeletePalette(screen->palette);
  DeleteBitmap(scroll);
  DeleteBitmap(screen);
  DeleteBitmap(trkselbt[0]);
  DeleteBitmap(trkselbt[1]);
  DeleteBitmap(trkselbt[2]);

  DeleteFont(small_font);
  DeleteFont(big_font);
  DeleteWaveScope(wavescope);

  AhxKillPlayer();
}

static __interrupt LONG AhxPlayerIntHandler() {
  /* Handle CIA Timer A interrupt. */
  if (ciaa->ciaicr & CIAICRF_TA)
    AhxInterrupt();

  return 0;
}

INTERRUPT(AhxPlayerInterrupt, 10, AhxPlayerIntHandler);

static void AhxSetTempo(UWORD tempo asm("d0")) {
  ciaa->ciatalo = tempo & 0xff;
  ciaa->ciatahi = tempo >> 8;
  ciaa->ciacra |= CIACRAF_START;
}

static void Init() {
  cp = NewCopList(1000);

  CopInit(cp);
  CopSetupGfxSimple(cp, MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);
  CopSetupBitplanes(cp, NULL, screen, DEPTH);
  CopSetupSprites(cp, sprptr);
  CopLoadPal(cp, screen->palette, 0);
  {
    WORD y, i = 0;

    for (y = 0; y < SCROLL_Y; y++) {
      if ((y == LABEL_Y + i * 10) && (i < MODULES)) {
        CopWait(cp, Y(y), X(0));
        module[i].bgcol = CopSetRGB(cp, COLOR, (i & 1) ? 0x311 : 0x411);
        i++;
      }
    }
  }
  CopWait(cp, Y(SCROLL_Y), 0);
  CopMove32(cp, bplpt[0], screen->planes[0] + SCROLL_Y * WIDTH / 8);
  CopMove32(cp, bplpt[1], scroll->planes[0]);
  CopMove32(cp, bplpt[2], screen->planes[1] + SCROLL_Y * WIDTH / 8);
  CopMove32(cp, bplpt[3], scroll->planes[1]);
  CopMove32(cp, bplpt[4], screen->planes[2] + SCROLL_Y * WIDTH / 8);
  CopMove32(cp, bplpt[5], scroll->planes[2]);
  CopMove16(cp, bplcon0, 
            BPLCON0_BPU(6) | BPLCON0_COLOR | MODE_LORES | MODE_DUALPF);
  CopEnd(cp);

  CopInsSet32(sprptr[0], pointer->data);
  UpdateSprite(pointer, X(0), Y(0));

  CopListActivate(cp);

  custom->dmacon = DMAF_SETCLR | DMAF_RASTER | DMAF_BLITTER | DMAF_SPRITE;

  KeyboardInit();
  MouseInit(0, 0, WIDTH - 1, HEIGHT - 1);

  GuiInit(gui, screen, small_font);
  GuiRedraw(gui);

  BitmapClearArea(screen, STRUCT(Area2D, SCOPE_X, SCOPE_Y, 64, 64));

  /* Enable only CIA Timer A interrupt. */
  ciaa->ciaicr = (UBYTE)(~CIAICRF_SETCLR);
  ciaa->ciaicr = CIAICRF_SETCLR | CIAICRF_TA;
  /* Run CIA Timer A in continuous / normal mode, increment every 10 cycles. */
  ciaa->ciacra &= (UBYTE)(~CIACRAF_RUNMODE & ~CIACRAF_INMODE & ~CIACRAF_PBON);

  if (AhxInitHardware((APTR)AhxSetTempo, AHX_KILL_SYSTEM))
    exit(10);

  AddIntServer(INTB_PORTS, &AhxPlayerInterrupt);

  {
    GuiEventT ev = { EV_GUI, WA_RELEASED, _b00 };
    PushEvent((EventT *)&ev);
    _b00->base.state |= WS_TOGGLED;
    GuiWidgetRedraw(gui, _b00);
  }
}

static void Kill() {
  custom->dmacon = DMAF_RASTER | DMAF_BLITTER | DMAF_SPRITE;

  RemIntServer(INTB_PORTS, &AhxPlayerInterrupt);

  AhxStopSong();
  AhxKillHardware();

  KeyboardKill();
  MouseKill();

  DeleteCopList(cp);
  DeleteSprite(pointer);
}

static BOOL HandleEvent() {
  EventT ev[1];

  if (!PopEvent(ev))
    return TRUE;

  if (ev->type == EV_KEY) {
   if (!(ev->key.modifier & MOD_PRESSED) && ev->key.code == KEY_ESCAPE)
      return FALSE;
  } else if (ev->type == EV_MOUSE) {
    GuiHandleMouseEvent(gui, &ev->mouse);
    UpdateSprite(pointer, X(ev->mouse.x), Y(ev->mouse.y));
  } else if (ev->type == EV_GUI) {
    WidgetT *wg = ev->gui.widget;

    if (wg->base.parent == _track_bt && ev->gui.action == WA_RELEASED) {
      WidgetT **buttons = GROUP(_track_bt)->widgets;
      WORD i = 0;

      while ((*buttons && *buttons != wg))
        buttons++, i++;

      SwitchModule(i);
    }
  }

  return TRUE;
}

static void Render() {
  WORD xo = normfx(SIN(frameCount * 32) * 8) + 8;
  WORD c0, c1;
  WORD i;

  if (active & 1)
    c0 = 0x311, c1 = 0xa44;
  else
    c0 = 0x411, c1 = 0xa44;

  if (xo > 15)
    xo = 15;

  CopInsSet16(module[active].bgcol, ColorTransition(c0, c1, xo));

  for (i = 0; i < 4; i++)
    WaveScopeUpdateChannel(wavescope, i, &Ahx.Public->VoiceTemp[i]);

  WaveScopeDrawChannel(wavescope, 0, screen, SCOPE_X, SCOPE_Y);
  WaveScopeDrawChannel(wavescope, 1, screen, SCOPE_X + 32, SCOPE_Y);
  WaveScopeDrawChannel(wavescope, 2, screen, SCOPE_X, SCOPE_Y + 32);
  WaveScopeDrawChannel(wavescope, 3, screen, SCOPE_X + 32, SCOPE_Y + 32);
}

EffectT Effect = { Load, UnLoad, Init, Kill, Render, HandleEvent };
