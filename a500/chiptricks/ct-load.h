#ifndef __CT_LOAD_H__
#define __CT_LOAD_H__

#include "common.h"

typedef struct {
  char *filename;
  char *title;
  char *author;
  char *note;
} ArtworkInfoT;

typedef struct {
  UWORD count;
  ArtworkInfoT info[0];
} ArtworkT;

__regargs ArtworkT *LoadArtwork(char *filename);
__regargs void DeleteArtwork(ArtworkT *artwork);

#endif
