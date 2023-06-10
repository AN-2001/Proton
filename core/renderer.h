/******************************************************************************\
*  renderer.h                                                                  *
*                                                                              *
*  An SDL based renderer.                                                      *
*                                                                              *
*              Written by Abed Na'ran                          May 2023        *
*                                                                              *
\******************************************************************************/
#ifndef RENDERER_H
#define RENDERER_H
#include "proton.h"
#include <SDL2/SDL.h>

void RendererBind(SDL_Renderer *Instance, void (*DrawingFunc)()); 
void RendererInit();
void RendererFree();
void ProtonRenderFrame();
void ProtonPushTransform();
void ProtonTranslate(PointStruct Point);
void ProtonScale(RealType Scale);
void ProtonRotate(RealType Ang);
void ProtonPopTransform();
PointStruct ProtonScreenToWorld(PointStruct ScreenPos);
PointStruct ProtonWorldToScreen(PointStruct WorldPos);
void ProtonSetFG(ColourStruct Col);
void ProtonSetBG(ColourStruct Col);
void ProtonDrawPoint(PointStruct Point);
void ProtonDrawLine(PointStruct P1, PointStruct P2);
void ProtonDrawCircle(PointStruct Centre, RealType Radius);
void ProtonFillCircle(PointStruct Centre, RealType Radius);
void ProtonDrawRect(PointStruct TopLeft, PointStruct BottomRight);
void ProtonFillRect(PointStruct TopLeft, PointStruct BottomRight);
void ProtonDrawText(const char *Text, PointStruct Pos);

#endif /* RENDERER_H */
