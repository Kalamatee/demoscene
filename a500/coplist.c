#include <exec/memory.h>
#include <proto/exec.h>

#include "coplist.h"
#include "hardware.h"

__regargs CopListT *NewCopList(UWORD length) {
  CopListT *list = AllocMem(sizeof(CopListT) + length * sizeof(CopInsT),
                            MEMF_CHIP|MEMF_CLEAR);

  list->length = length;

  CopInit(list);

  return list;
}

__regargs void DeleteCopList(CopListT *list) {
  FreeMem(list, sizeof(CopListT) + list->length * sizeof(CopInsT));
}

__regargs void CopListActivate(CopListT *list) {
  WaitVBlank();
  /* Write copper list address. */
  custom->cop1lc = (ULONG)list->entry;
  /* Activate it immediately */
  custom->copjmp1 = 0;
  /* Enable copper DMA */
  custom->dmacon = DMAF_MASTER | DMAF_COPPER | DMAF_SETCLR;
}

__regargs void CopInit(CopListT *list) {
  list->flags = 0;
  list->curr = list->entry;
}

__regargs CopInsT *CopWait(CopListT *list, UWORD vp, UWORD hp) {
  UWORD *ins = (UWORD *)list->curr;

  if ((vp >= 256) && (!list->flags)) {
    *((ULONG *)ins)++ = 0xffdffffe;
    list->flags |= 1;
  }

  {
    CopInsT *ptr = (CopInsT *)ins;

    *ins++ = (vp << 8) | (hp & 0xff) | 1;
    *ins++ = 0xfffe;

    list->curr = (CopInsT *)ins;

    return ptr;
  }
}

__regargs CopInsT *CopMove16(CopListT *list, UWORD reg, UWORD data) {
  UWORD *ins = (UWORD *)list->curr;

  *ins++ = reg & 0x01fe;
  *ins++ = data;

  {
    CopInsT *ptr = list->curr;
    list->curr = (CopInsT *)ins;
    return ptr;
  }
}

__regargs CopInsT *CopMove32(CopListT *list, UWORD reg, ULONG data) {
  UWORD *ins = (UWORD *)list->curr;

  reg &= 0x01fe;

  *ins++ = reg;
  *ins++ = data >> 16;
  *ins++ = reg + 2;
  *ins++ = data;

  {
    CopInsT *ptr = list->curr;
    list->curr = (CopInsT *)ins;
    return ptr;
  }
}

__regargs void CopEnd(CopListT *list) {
  ULONG *ins = (ULONG *)list->curr;

  *ins++ = 0xfffffffe;

  list->curr = (CopInsT *)ins;
}