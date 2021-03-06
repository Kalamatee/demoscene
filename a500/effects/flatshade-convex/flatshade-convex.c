#include "startup.h"
#include "blitter.h"
#include "coplist.h"
#include "3d.h"
#include "fx.h"
#include "ffp.h"
#include "ilbm.h"
#include "tasks.h"

STRPTR __cwdpath = "data";

#define WIDTH  256
#define HEIGHT 256
#define DEPTH  4

static PaletteT *palette;
static Mesh3D *mesh;
static Object3D *cube;
static CopListT *cp;
static CopInsT *bplptr[DEPTH];
static BitmapT *screen[2];
static WORD active;

static void Load() {
  // mesh = LoadMesh3D("ball.3d", SPFlt(110));
  mesh = LoadMesh3D("pilka.3d", SPFlt(65));
  CalculateVertexFaceMap(mesh);
  CalculateFaceNormals(mesh);
  CalculateEdges(mesh);
  palette = LoadPalette("flatshade-pal.ilbm");
}

static void UnLoad() {
  DeletePalette(palette);
  DeleteMesh3D(mesh);
}

static void MakeCopperList(CopListT *cp) {
  CopInit(cp);
  CopSetupGfxSimple(cp, MODE_LORES, DEPTH, X(32), Y(0), WIDTH, HEIGHT);
  CopSetupBitplanes(cp, bplptr, screen[0], DEPTH);
  CopLoadPal(cp, palette, 0);
  CopEnd(cp);
}

static void Init() {
  cube = NewObject3D(mesh);
  cube->translate.z = fx4i(-250);

  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH);

  cp = NewCopList(80);
  MakeCopperList(cp);
  CopListActivate(cp);
  EnableDMA(DMAF_BLITTER | DMAF_RASTER | DMAF_BLITHOG);
}

static void Kill() {
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
  DeleteCopList(cp);
  DeleteObject3D(cube);
}

static __regargs void UpdateEdgeVisibilityConvex(Object3D *object) {
  BYTE *vertexFlags = object->vertexFlags;
  BYTE *edgeFlags = object->edgeFlags;
  BYTE *faceFlags = object->faceFlags;
  IndexListT **faces = object->mesh->face;
  IndexListT *face = *faces++;
  IndexListT **faceEdges = object->mesh->faceEdge;
  IndexListT *faceEdge = *faceEdges++;

  bzero(vertexFlags, object->mesh->vertices);
  bzero(edgeFlags, object->mesh->edges);

  do {
    BYTE f = *faceFlags++;

    if (f >= 0) {
      WORD n = face->count - 3;
      WORD *vi = face->indices;
      WORD *ei = faceEdge->indices;

      /* Face has at least (and usually) three vertices / edges. */
      vertexFlags[*vi++] = -1;
      edgeFlags[*ei++] ^= f;
      vertexFlags[*vi++] = -1;
      edgeFlags[*ei++] ^= f;

      do {
        vertexFlags[*vi++] = -1;
        edgeFlags[*ei++] ^= f;
      } while (--n != -1);
    }

    faceEdge = *faceEdges++;
    face = *faces++;
  } while (face);
}

#define MULVERTEX1(D, E) {               \
  WORD t0 = (*v++) + y;                  \
  WORD t1 = (*v++) + x;                  \
  LONG t2 = (*v++) * z;                  \
  v++;                                   \
  D = ((t0 * t1 + t2 - x * y) >> 4) + E; \
}

#define MULVERTEX2(D) {                  \
  WORD t0 = (*v++) + y;                  \
  WORD t1 = (*v++) + x;                  \
  LONG t2 = (*v++) * z;                  \
  WORD t3 = (*v++);                      \
  D = normfx(t0 * t1 + t2 - x * y) + t3; \
}

static __regargs void TransformVertices(Object3D *object) {
  Matrix3D *M = &object->objectToWorld;
  WORD *v = (WORD *)M;
  WORD *src = (WORD *)object->mesh->vertex;
  WORD *dst = (WORD *)object->vertex;
  BYTE *flags = object->vertexFlags;
  register WORD n asm("d7") = object->mesh->vertices - 1;

  LONG m0 = (M->x << 8) - ((M->m00 * M->m01) >> 4);
  LONG m1 = (M->y << 8) - ((M->m10 * M->m11) >> 4);

  WORD cnt = 0;

  /* WARNING! This modifies camera matrix! */
  M->z -= normfx(M->m20 * M->m21);

  /*
   * A = m00 * m01
   * B = m10 * m11
   * C = m20 * m21 
   * yx = y * x
   *
   * (m00 + y) * (m01 + x) + m02 * z - yx + (mx - A)
   * (m10 + y) * (m11 + x) + m12 * z - yx + (my - B)
   * (m20 + y) * (m21 + x) + m22 * z - yx + (mz - C)
   */

  do {
    if (*flags++) {
      WORD x = *src++;
      WORD y = *src++;
      WORD z = *src++;
      LONG xp, yp;
      WORD zp;

      pushl(v);
      MULVERTEX1(xp, m0);
      MULVERTEX1(yp, m1);
      MULVERTEX2(zp);
      popl(v);

      *dst++ = div16(xp, zp) + WIDTH / 2;  /* div(xp * 256, zp) */
      *dst++ = div16(yp, zp) + HEIGHT / 2; /* div(yp * 256, zp) */
      *dst++ = zp;

      src++;
      dst++;
      cnt++;
    } else {
      src += 4;
      dst += 4;
    }
  } while (--n != -1);
}

static __regargs void DrawObject(BitmapT *screen, Object3D *object,
                                 CustomPtrT custom asm("a6"))
{
  WORD *edge = (WORD *)object->mesh->edge;
  BYTE *edgeFlags = object->edgeFlags;
  Point3D *point = object->vertex;
  WORD n = object->mesh->edges - 1;
  APTR planes = screen->planes[0];

  WaitBlitter();
  custom->bltafwm = -1;
  custom->bltalwm = -1;
  custom->bltadat = 0x8000;
  custom->bltbdat = 0xffff; /* Line texture pattern. */
  custom->bltcmod = WIDTH / 8;
  custom->bltdmod = WIDTH / 8;

  do {
    BYTE f = *edgeFlags++;

    if (f) {
      WORD x0, y0, x1, y1;
      APTR data;

      {
        WORD *p0 = (APTR)point + *edge++;
        x0 = *p0++;
        y0 = *p0++;
      }
      
      {
        WORD *p1 = (APTR)point + *edge++;
        x1 = *p1++;
        y1 = *p1++;
      }

      if (y0 > y1) {
        swapr(x0, x1);
        swapr(y0, y1);
      }

      {
        WORD dmax = x1 - x0;
        WORD dmin = y1 - y0;
        WORD derr;
        UWORD bltcon1 = LINEMODE | ONEDOT;

        if (dmax < 0)
          dmax = -dmax;

        if (dmax >= dmin) {
          if (x0 >= x1)
            bltcon1 |= (AUL | SUD);
          else
            bltcon1 |= SUD;
        } else {
          if (x0 >= x1)
            bltcon1 |= SUL;
          swapr(dmax, dmin);
        }

        {
          WORD y0_ = y0 << 5;
          WORD x0_ = x0 >> 3;
          WORD start = (y0_ + x0_) & ~1;
          data = planes + start;
        }

        dmin <<= 1;
        derr = dmin - dmax;
        if (derr < 0)
          bltcon1 |= SIGNFLAG;
        bltcon1 |= rorw(x0 & 15, 4);

        {
          UWORD bltcon0 = rorw(x0 & 15, 4) | BC0F_LINE_EOR;
          UWORD bltamod = derr - dmax;
          UWORD bltbmod = dmin;
          UWORD bltsize = (dmax << 6) + 66;
          APTR bltapt = (APTR)(LONG)derr;
          
#define DRAWLINE()                      \
          WaitBlitter();                \
          custom->bltcon0 = bltcon0;    \
          custom->bltcon1 = bltcon1;    \
          custom->bltcpt = data;        \
          custom->bltapt = bltapt;      \
          custom->bltdpt = planes;      \
          custom->bltbmod = bltbmod;    \
          custom->bltamod = bltamod;    \
          custom->bltsize = bltsize;

          if (f & 1) { DRAWLINE(); }
          data += WIDTH * HEIGHT / 8;
          if (f & 2) { DRAWLINE(); }
          data += WIDTH * HEIGHT / 8;
          if (f & 4) { DRAWLINE(); }
          data += WIDTH * HEIGHT / 8;
          if (f & 8) { DRAWLINE(); }
        }
      }
    } else {
      edge += 2;
    }
  } while (--n != -1);
}

static __regargs void BitmapClearFast(BitmapT *dst) {
  UWORD height = (WORD)dst->height * (WORD)dst->depth;
  UWORD bltsize = (height << 6) | (dst->bytesPerRow >> 1);
  APTR bltpt = dst->planes[0];

  WaitBlitter();

  custom->bltcon0 = DEST | A_TO_D;
  custom->bltcon1 = 0;
  custom->bltafwm = -1;
  custom->bltalwm = -1;
  custom->bltadat = 0;
  custom->bltdmod = 0;
  custom->bltdpt = bltpt;
  custom->bltsize = bltsize;
}

static __regargs void BitmapFillFast(BitmapT *dst) {
  APTR bltpt = dst->planes[0] + (dst->bplSize * DEPTH) - 2;
  UWORD bltsize = (0 << 6) | (WIDTH >> 4);

  WaitBlitter();

  custom->bltapt = bltpt;
  custom->bltdpt = bltpt;
  custom->bltamod = 0;
  custom->bltdmod = 0;
  custom->bltcon0 = (SRCA | DEST) | A_TO_D;
  custom->bltcon1 = BLITREVERSE | FILL_XOR;
  custom->bltafwm = -1;
  custom->bltalwm = -1;
  custom->bltsize = bltsize;

  WaitBlitter();
}

static void Render() {
  LONG lines = ReadLineCounter();
 
  BitmapClearFast(screen[active]);

  /* ball: 92 points, 180 polygons, 270 edges */
   cube->rotate.x = cube->rotate.y = cube->rotate.z = frameCount * 8;

  {
    // LONG lines = ReadLineCounter();
    UpdateObjectTransformation(cube); // 18 lines
    UpdateFaceVisibility(cube); // 211 lines O(faces)
    UpdateEdgeVisibilityConvex(cube); // 78 lines O(edge)
    TransformVertices(cube); // 89 lines O(vertex)
    // Log("transform: %ld\n", ReadLineCounter() - lines);
  }

  {
    // LONG lines = ReadLineCounter();
    DrawObject(screen[active], cube, custom); // 237 lines
    // Log("draw: %ld\n", ReadLineCounter() - lines);
  }

  {
    // LONG lines = ReadLineCounter();
    BitmapFillFast(screen[active]); // 287 lines
    // Log("fill: %ld\n", ReadLineCounter() - lines);
  }

  Log("all: %ld\n", ReadLineCounter() - lines);

  CopUpdateBitplanes(bplptr, screen[active], DEPTH);
  TaskWait(VBlankEvent);
  active ^= 1;
}

EffectT Effect = { Load, UnLoad, Init, Kill, Render };
