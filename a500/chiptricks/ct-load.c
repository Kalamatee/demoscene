#include "io.h"
#include "memory.h"
#include "reader.h"
#include "ct-load.h"

typedef struct {
  const char *name;
  BOOL (*func)(char **, ArtworkT *);
} ParserT;

static char *ConsumeString(char **data) {
  static char text[80];
  WORD len;
  if ((len = ReadString(data, text, sizeof(text))) && EndOfLine(data)) {
    char *ptr = MemAlloc(len + 1, MEMF_PUBLIC);
    strcpy(ptr, text);
    return ptr;
  }
  return NULL;
}

static BOOL ParseArtwork(char **data, ArtworkT *artwork) {
  ArtworkInfoT *info = &artwork->info[artwork->count];
  return (info->filename = ConsumeString(data)) ? TRUE : FALSE;
}

static BOOL ParseTitle(char **data, ArtworkT *artwork) {
  ArtworkInfoT *info = &artwork->info[artwork->count];
  return (info->title = ConsumeString(data)) ? TRUE : FALSE;
}

static BOOL ParseAuthor(char **data, ArtworkT *artwork) {
  ArtworkInfoT *info = &artwork->info[artwork->count];
  return (info->author = ConsumeString(data)) ? TRUE : FALSE;
}

static BOOL ParseNote(char **data, ArtworkT *artwork) {
  ArtworkInfoT *info = &artwork->info[artwork->count];
  char *begin, *end;

  if (!EndOfLine(data))
    return FALSE;

  SkipLine(data), begin = *data, end = *data;

  while (**data) {
    if (MatchString(data, "@end")) {
      UWORD len = end - begin;
      begin[len - 1] = '\0';
      info->note = MemAlloc(len, MEMF_PUBLIC);
      strcpy(info->note, begin);
      artwork->count++;
      return TRUE;
    }
    SkipLine(data), end = *data;
  }

  return FALSE;
}

static ParserT TopLevelParser[] = {
  { "@artwork", &ParseArtwork },
  { "@title", &ParseTitle },
  { "@author", &ParseAuthor },
  { "@note", &ParseNote },
  { NULL, NULL }
};

__regargs ArtworkT *LoadArtwork(char *filename) {
  char *file = LoadFile(filename, MEMF_PUBLIC);
  char *data = file;
  ArtworkT *artwork;
  WORD count;

  Log("[Artwork] Parsing '%s' file\n", filename);

  if (!NextLine(&data) ||
      !MatchString(&data, "@count") ||
      !ReadShort(&data, &count))
    return NULL;

  artwork = MemAlloc(sizeof(ArtworkT) + count * sizeof(ArtworkInfoT),
                     MEMF_PUBLIC|MEMF_CLEAR);

  while (NextLine(&data)) {
    ParserT *parser = TopLevelParser;
    
    for (; parser->name; parser++) {
      if (!MatchString(&data, parser->name))
        continue;
      if (parser->func(&data, artwork))
        break;
      Log("[Artwork] Parse error at position %ld!\n", (LONG)(data - file));
      DeleteArtwork(artwork);
      MemFree(file);
      return NULL;
    }
  }

  MemFree(file);
  return artwork;
}

__regargs void DeleteArtwork(ArtworkT *artwork) {
}
