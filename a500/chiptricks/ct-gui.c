#include "ct-gui.h"
#include "gfx.h"
#include "mouse.h"
#include "bltop.h"

static inline void PushGuiEvent(WidgetT *wg, WORD event) {
  GuiEventT ev = { EV_GUI, event, wg };
  PushEvent((EventT *)&ev);
}

inline void GuiWidgetRedraw(GuiStateT *gui, WidgetT *wg);

static void GroupRedraw(GuiStateT *gui, GroupT *wg) {
  WidgetT **widgets = wg->widgets;
  WidgetT *w;

  while ((w = *widgets++))
    GuiWidgetRedraw(gui, w);
}

static void LabelRedraw(GuiStateT *gui, LabelT *wg) {
  Area2D area = { wg->area.x, wg->area.y + 1, wg->area.w, wg->area.h };

  Log("LabelRedraw: '%s'\n", wg->text);
  BitmapSetArea(gui->screen, &wg->area, 8);
  DrawText(&area, wg->text);
}

static void ButtonRedraw(GuiStateT *gui, ButtonT *wg) {
  WORD i, j = 0;

  if (wg->state & WS_ACTIVE) j = 1;
  if (wg->state & WS_PRESSED) j = 2;
  if ((wg->state & WS_TOGGLED) && !(wg->state & WS_ACTIVE)) j = 2;

  for (i = 0; i < gui->screen->depth; i++)
    BlitterCopy(gui->screen, i, wg->area.x, wg->area.y, wg->images[j], i);
}

static void ButtonPress(GuiStateT *gui, ButtonT *wg) {
  wg->state |= WS_PRESSED;
  ButtonRedraw(gui, wg);
  PushGuiEvent((WidgetT *)wg, WA_PRESSED);
}

static void ButtonRelease(GuiStateT *gui, ButtonT *wg) {
  wg->state &= ~WS_PRESSED;
  ButtonRedraw(gui, wg);
  if (gui->lastPressed == (WidgetBaseT *)wg)
    PushGuiEvent((WidgetT *)wg, WA_RELEASED);
}

static void RadioButtonRelease(GuiStateT *gui, ButtonT *wg) {
  wg->state &= ~WS_PRESSED;
  if (gui->lastPressed == (WidgetBaseT *)wg) {
    PushGuiEvent((WidgetT *)wg, WA_RELEASED);
    if (!(wg->state & WS_TOGGLED)) {
      WidgetT **widgets = GROUP(wg->parent)->widgets;
      WidgetT *child;

      while ((child = *widgets++)) {
        if (child->base.state & WS_TOGGLED) {
          child->base.state &= ~WS_TOGGLED;
          GuiWidgetRedraw(gui, child);
        }
      }

      wg->state |= WS_TOGGLED;
    }
  }
  ButtonRedraw(gui, wg);
}

static void ButtonLeave(GuiStateT *gui, ButtonT *wg) {
  wg->state &= ~WS_ACTIVE;
  wg->state &= ~WS_PRESSED;
  ButtonRedraw(gui, wg);
  PushGuiEvent((WidgetT *)wg, WA_LEFT);
}

static void ButtonEnter(GuiStateT *gui, ButtonT *wg) {
  wg->state |= WS_ACTIVE;
  if (gui->lastPressed == (WidgetBaseT *)wg)
    wg->state |= WS_PRESSED;
  ButtonRedraw(gui, wg);
  PushGuiEvent((WidgetT *)wg, WA_ENTERED);
}

/* Dynamic function dispatch. */
static void WidgetDummyFunc(GuiStateT *gui, ButtonT *wg) {}

inline void GuiWidgetRedraw(GuiStateT *gui, WidgetT *wg) {
  static WidgetFuncT WidgetRedrawFunc[WT_LAST] = {
    (WidgetFuncT)GroupRedraw, 
    (WidgetFuncT)LabelRedraw, 
    (WidgetFuncT)ButtonRedraw,
    (WidgetFuncT)ButtonRedraw,
  };
  WidgetRedrawFunc[(wg)->type](gui, (WidgetT *)wg);
}

static WidgetFuncT WidgetEnterFunc[WT_LAST] = {
  (WidgetFuncT)WidgetDummyFunc,
  (WidgetFuncT)WidgetDummyFunc,
  (WidgetFuncT)ButtonEnter,
  (WidgetFuncT)ButtonEnter,
};

#define WidgetEnter(gui, wg) \
  WidgetEnterFunc[(wg)->type]((gui), (WidgetT *)wg)

static WidgetFuncT WidgetLeaveFunc[WT_LAST] = {
  (WidgetFuncT)WidgetDummyFunc,
  (WidgetFuncT)WidgetDummyFunc,
  (WidgetFuncT)ButtonLeave,
  (WidgetFuncT)ButtonLeave,
};

#define WidgetLeave(gui, wg) \
  WidgetLeaveFunc[(wg)->type]((gui), (WidgetT *)wg)

static WidgetFuncT WidgetPressFunc[WT_LAST] = {
  (WidgetFuncT)WidgetDummyFunc,
  (WidgetFuncT)WidgetDummyFunc,
  (WidgetFuncT)ButtonPress,
  (WidgetFuncT)ButtonPress,
};

#define WidgetPress(gui, wg) \
  WidgetPressFunc[(wg)->type]((gui), (WidgetT *)wg)

static WidgetFuncT WidgetReleaseFunc[WT_LAST] = {
  (WidgetFuncT)WidgetDummyFunc,
  (WidgetFuncT)WidgetDummyFunc,
  (WidgetFuncT)ButtonRelease,
  (WidgetFuncT)RadioButtonRelease,
};

#define WidgetRelease(gui, wg) \
  WidgetReleaseFunc[(wg)->type]((gui), (WidgetT *)wg)

static WidgetT *FindWidgetByMouse(WidgetT *wg, WORD x, WORD y) {
  if (wg->type == WT_GROUP) {
    WidgetT **widgets = GROUP(wg)->widgets;
    WidgetT *child;

    while ((child = *widgets++))
      if ((wg = FindWidgetByMouse(child, x, y)))
        return wg;

    return NULL;
  }

  return InsideArea(x, y, &wg->base.area) ? wg : NULL;
}

static void InitWidget(WidgetT *wg, WidgetT *parent) {
  if (wg->type == WT_GROUP) {
    WidgetT **widgets = GROUP(wg)->widgets;
    WidgetT *child;

    while ((child = *widgets++))
      InitWidget(child, wg);
  } else {
    wg->base.parent = parent;
  }
}

void GuiInit(GuiStateT *gui, BitmapT *screen, FontT *font) {
  gui->screen = screen;
  gui->font = font;

  InitWidget(gui->root, NULL);
}

void GuiRedraw(GuiStateT *gui) {
  DrawTextSetup(gui->screen, 0, gui->font);
  GuiWidgetRedraw(gui, gui->root);
}

void GuiHandleMouseEvent(GuiStateT *gui, struct MouseEvent *mouse) {
  WidgetBaseT *wg = gui->lastEntered;

  if (wg) {
    if (InsideArea(mouse->x, mouse->y, &wg->area)) {
      if (mouse->button & LMB_PRESSED) {
        WidgetPress(gui, wg);
        gui->lastPressed = wg;
      } else if (mouse->button & LMB_RELEASED) {
        WidgetRelease(gui, wg);
        gui->lastPressed = NULL;
      }
      return;
    }
    WidgetLeave(gui, wg);
  }

  if (mouse->button & LMB_RELEASED)
    gui->lastPressed = NULL;
  
  /* Find the widget a pointer is hovering on. */
  wg = (WidgetBaseT *)FindWidgetByMouse(gui->root, mouse->x, mouse->y);

  gui->lastEntered = wg;

  if (wg)
    WidgetEnter(gui, wg);
}
