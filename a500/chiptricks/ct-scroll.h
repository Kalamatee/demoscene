#ifndef __CT_SCROLL_H__
#define __CT_SCROLL_H__

#include "common.h"
#include "coplist.h"
#include "font.h"

void ScrollInit(FontT *font, CopInsT **planept);
void ScrollKill(void);
void ScrollReset(char *text);
void ScrollRender(WORD frameCount);

#endif
