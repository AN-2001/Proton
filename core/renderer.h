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
void RendererRenderFrame();

void RendererPushTransform();
void RendererTranslate(PointStruct Point);
void RendererScale(RealType Scale);
void RendererRotate(RealType Ang);
void RendererPopTransform();
PointStruct RendererScreenToWorld(PointStruct ScreenPos);
PointStruct RendererWorldToScreen(PointStruct WorldPos);

void RendererSetFG(IntType r, IntType g, IntType b);
void RendererSetBG(IntType r, IntType g, IntType b);
void RendererSetFGA(IntType r, IntType g, IntType b, IntType a);
void RendererSetBGA(IntType r, IntType g, IntType b, IntType a);
void RendererDrawPoint(PointStruct Point);
void RendererDrawLine(PointStruct P1, PointStruct P2);
void RendererDrawCircle(PointStruct Centre, RealType Radius);
void RendererFillCircle(PointStruct Centre, RealType Radius);
void RendererDrawRectangle(PointStruct TopLeft, PointStruct BottomRight);
void RendererFillRectangle(PointStruct TopLeft, PointStruct BottomRight);
void RendererDrawText(const char *Text, PointStruct Pos);

#endif /* RENDERER_H */
