#ifndef __SYSTEM_DISPLAY_H__
#define __SYSTEM_DISPLAY_H__

#include <graphics/gfx.h>
#include <graphics/view.h>

#include "gfx/canvas.h"

struct DBufRaster {
  struct ViewPort *ViewPort;
  struct DBufInfo *DBufInfo;
  struct BitMap *BitMap;

  canvas_t *Canvas;

  LONG CurrentBitMap;
  BOOL SafeToWrite;
  BOOL SafeToSwap;
};

struct DBufRaster *NewDBufRaster(SHORT width, SHORT height, SHORT depth);
void DeleteDBufRaster(struct DBufRaster *raster);
void WaitForSafeToWrite(struct DBufRaster *raster);
void WaitForSafeToSwap(struct DBufRaster *raster);
void DBufRasterSwap(struct DBufRaster *raster);

void ConfigureViewPort(struct ViewPort *viewPort);
void LoadPalette(struct ViewPort *viewPort, UBYTE *components,
                 UWORD start, UWORD count);

struct View *NewView();
void DeleteView(struct View *view);
void ApplyView(struct View *view, struct ViewPort *viewPort);
void SaveOrigView();
void RestoreOrigView();

#endif