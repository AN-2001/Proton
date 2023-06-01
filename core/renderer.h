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
void RendererPushTransform(PointStruct Translate, RealType Scale); 
void RendererPopTransform();
void RendererSetFGColour(IntType r, IntType g, IntType b);
void RendererSetBGColour(IntType r, IntType g, IntType b);
void RendererSetFGAColour(IntType r, IntType g, IntType b, IntType a);
void RendererSetBGAColour(IntType r, IntType g, IntType b, IntType a);
void RendererDrawPoint(PointStruct Point);
void RendererDrawLine(PointStruct P1, PointStruct P2);
void RendererDrawCircle(PointStruct Centre, RealType Radius);
void RendererFillCircle(PointStruct Centre, RealType Radius);
void RendererDrawRectangle(PointStruct TopLeft, PointStruct BottomRight);
void RendererFillRectangle(PointStruct TopLeft, PointStruct BottomRight);
void RendererDrawText(const char *Text, PointStruct Pos);
PointStruct RendererScreenToWorld(PointStruct ScreenPos);
PointStruct RendererWorldToScreen(PointStruct WorldPos);

#endif /* RENDERER_H */
