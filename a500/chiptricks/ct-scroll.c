#include "gfx.h"
#include "blitter.h"
#include "chiptricks.h"
#include "ct-scroll.h"

static BitmapT *screen;
static BitmapT *scratch;
static WORD char_curr, char_width, char_x;
static char *scroll_text;
static FontT *scroll_font;

void ScrollInit(FontT *font, CopInsT **planept) {
  screen = NewBitmapCustom(WIDTH + 16, SCROLL_H, 3, BM_DISPLAYABLE|BM_CLEAR);
  scratch = NewBitmapCustom(WIDTH + 16, SCROLL_H, 3, BM_DISPLAYABLE|BM_CLEAR);

  CopInsSet32(planept[0], screen->planes[0]);
  CopInsSet32(planept[1], screen->planes[1]);
  CopInsSet32(planept[2], screen->planes[2]);

  scroll_font = font;
}

void ScrollKill(void) {
  DeleteBitmap(screen);
  DeleteBitmap(scratch);
}

void ScrollReset(char *text) {
  char_x = 0;
  char_width = 0;
  char_curr = 0;
  scroll_text = text;

  Log("ScrollText: %s\n", text);

  BitmapClear(screen);
  BitmapClear(scratch);
}

void ScrollRender(WORD frameCount) {
  Area2D area;

  if (char_x >= char_width) {
    FontCharT *charmap = scroll_font->charmap;
    WORD c = scroll_text[char_curr];

    if (c)
      char_curr++;

    char_x = 0;
    
    area = (Area2D){0, 0, WIDTH, SCROLL_H};
    BitmapCopyArea(scratch, 0, 0, screen, &area);

    if (c > ' ') {
      /* insert character from font bitmap */ 
      c -= 33;
      char_width = charmap[c].width;
      area = (Area2D){0, charmap[c].y, 16, scroll_font->height};
      BitmapCopyArea(scratch, WIDTH, 0, scroll_font->data, &area);
    } else {
      /* insert empty character */
      char_width = scroll_font->space; 
      area = (Area2D){WIDTH, 0, 16, scroll_font->height};
      BitmapClearArea(scratch, &area);
    }
  }
  
  area = (Area2D){char_x, 0, WIDTH, SCROLL_H};
  BitmapCopyArea(screen, 0, 0, scratch, &area);
  char_x++;
}
