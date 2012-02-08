#include <math.h>

#include "gfx/line.h"
#include "gfx/transformations.h"
#include "std/resource.h"

#include "system/c2p.h"
#include "system/debug.h"
#include "system/display.h"
#include "system/memory.h"
#include "system/vblank.h"

#include "frame_tools.h"

const int WIDTH = 320;
const int HEIGHT = 256;
const int DEPTH = 8;

static CanvasT *Canvas;

static PointT *Cross;
static PointT CrossToDraw[12];

/*
 * Set up display function.
 */
bool SetupDisplay() {
  return InitDisplay(WIDTH, HEIGHT, DEPTH);
}

/*
 * Set up effect function.
 */
void SetupEffect() {
  Cross = GetResource("cross");
  Canvas = NewCanvas(WIDTH, HEIGHT);

  TS_Init();
}

/*
 * Tear down effect function.
 */
void TearDownEffect() {
  TS_End();

  DeleteCanvas(Canvas);
}

/*
 * Effect rendering functions.
 */
void RenderVector(int frameNumber) {
  float s = sin(frameNumber * 3.14159265f / 45.0f);
  float c = cos(frameNumber * 3.14159265f / 90.0f);

  TS_Reset();
  TS_PushTranslation2D(-1.5f, -1.5f);
  TS_PushScaling2D(20.0f + 10.0f * s, 20.0f + 10.0f * s);
  TS_Compose2D();
  TS_PushRotation2D((float)(frameNumber * -3));
  TS_Compose2D();
  TS_PushTranslation2D((float)(WIDTH/2) + c * (WIDTH/4), (float)(HEIGHT/2));
  TS_Compose2D();

  M2D_Transform(CrossToDraw, Cross, 12, TS_GetMatrix2D(1));

  CanvasFill(Canvas, 0);
  DrawPolyLine(Canvas, CrossToDraw, 12, TRUE);
}

void RenderChunky(int frameNumber) {
  c2p1x1_8_c5_bm(GetCanvasPixelData(Canvas), GetCurrentBitMap(), WIDTH, HEIGHT, 0, 0);
}

/*
 * Main loop.
 */
void MainLoop() {
  SetVBlankCounter(0);

  while (GetVBlankCounter() < 500) {
    int frameNumber = GetVBlankCounter();

    RenderVector(frameNumber);
    RenderChunky(frameNumber);
    RenderFrameNumber(frameNumber);

    DisplaySwap();
  }
}
