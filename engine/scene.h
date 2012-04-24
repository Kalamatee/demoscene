#ifndef __ENGINE_SCENE_H__
#define __ENGINE_SCENE_H__

#include "engine/object.h"

typedef struct Scene SceneT;

SceneT *NewScene();
void DeleteScene(SceneT *self);
void SceneAddObject(SceneT *self, SceneObjectT *object);
MatrixStack3D *GetObjectTranslation(SceneT *self, const StrT name);
void RenderScene(SceneT *self, CanvasT *canvas);

#define RSC_SCENE(NAME) \
  AddRscSimple(NAME, NewScene(), (FreeFuncT)DeleteScene)

#endif
