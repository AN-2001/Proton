#include "renderer.h"
#include "SDL2/SDL_ttf.h"
#include "SDL_mutex.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "transform.h"
#include "proton.h"
#include <math.h>
#include <memory.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include "GL/gl.h"

#define CIRCLE_RES (8)
#define TRANSFORM_STACK_SIZE (256)

static SDL_Renderer
    *RendererInstance = NULL;
static ColourStruct
    Foreground = COLOUR(255, 255, 255, 255),
    Background = COLOUR(0, 0, 0, 255);
static void 
    (*DrawingFunc)() = NULL;
static IntType
    TransformTop = -1;
static MatType
    TransformStack[TRANSFORM_STACK_SIZE];
const static MatType 
        Identity = {
            {1.f, 0.f, 0.f},
            {0.f, 1.f, 0.f},
        };
static MatType Transform, TransformInverse;
static TTF_Font
    *Font = NULL;
    
void RendererBind(SDL_Renderer *Instance, void (*Func)())
{
    RendererInstance = Instance;
    DrawingFunc = Func;
}

void RendererInit()
{
    TTF_Init();
    Font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 20);
    //TTF_SetFontOutline(Font, 1);
    TTF_SetFontStyle(Font, TTF_STYLE_BOLD);
    SDL_SetRenderDrawBlendMode(RendererInstance, SDL_BLENDMODE_BLEND);
    memcpy(Transform, Identity, sizeof(Transform));
    memcpy(TransformInverse, Identity, sizeof(TransformInverse));
}

void RendererFree()
{
    TTF_CloseFont(Font);
}

void RendererPushTransform(PointStruct Translate, RealType Scale)
{
    TransformTop = MIN(TransformTop + 1, TRANSFORM_STACK_SIZE - 1);
    TransformStack[TransformTop][0][0] =
    TransformStack[TransformTop][1][1] = Scale; 
    TransformStack[TransformTop][0][2] = Translate.x;
    TransformStack[TransformTop][1][2] = Translate.y;
    TransformMult(TransformStack[TransformTop], Transform, Transform);
    TransformInvert(Transform, TransformInverse);
}

void RendererPopTransform()
{
    MatType Inverse;

    TransformInvert(TransformStack[TransformTop], Inverse);
    TransformMult(Inverse, Transform, Transform);
    TransformInvert(Transform, TransformInverse);
    TransformTop = MAX(TransformTop - 1, 0);
}

void RendererRenderFrame() 
{
    SDL_SetRenderDrawColor(RendererInstance, Background.r * 255,
                                             Background.g * 255,
                                             Background.b * 255,
                                             Background.a * 255);
    SDL_RenderClear(RendererInstance);
    DrawingFunc();
    SDL_RenderPresent(RendererInstance);
}

void RendererSetFGColour(IntType r, IntType g, IntType b)
{
    Foreground = COLOUR(r, g, b, 255);
}

void RendererSetBGColour(IntType r, IntType g, IntType b)
{
    Background = COLOUR(r, g, b, 255);
}

void RendererSetFGAColour(IntType r, IntType g, IntType b, IntType a)
{
    Foreground = COLOUR(r, g, b, a);
}

void RendererSetBGAColour(IntType r, IntType g, IntType b, IntType a)
{
    Background = COLOUR(r, g, b, a);
}

void RendererDrawPoint(PointStruct Point)
{
    Point = TRANS_APPLY_POINT(Transform, Point);
    SDL_SetRenderDrawColor(RendererInstance, Foreground.r * 255,
                                             Foreground.g * 255,
                                             Foreground.g * 255,
                                             Foreground.a * 255);
    SDL_RenderDrawPoint(RendererInstance, Point.x, Point.y);
}

void RendererDrawLine(PointStruct P1, PointStruct P2)
{
    P1 = TRANS_APPLY_POINT(Transform, P1);
    P2 = TRANS_APPLY_POINT(Transform, P2);
    SDL_SetRenderDrawColor(RendererInstance, Foreground.r * 255,
                                             Foreground.g * 255,
                                             Foreground.b * 255,
                                             Foreground.a * 255);
    SDL_RenderDrawLine(RendererInstance, P1.x, P1.y, P2.x, P2.y);
}

void RendererDrawCircle(PointStruct Centre, RealType Radius)
{
    IntType i;
    RealType t1, t2;
    PointStruct Tmp1, Tmp2,
        Rad = POINT(Radius, Radius);

    Centre = TRANS_APPLY_POINT(Transform, Centre);
    Rad = TRANS_APPLY_VEC(Transform, Rad);
    SDL_SetRenderDrawColor(RendererInstance, Foreground.r * 255,
                                             Foreground.g * 255,
                                             Foreground.b * 255,
                                             Foreground.a * 255);
    for (i = 0; i < CIRCLE_RES; i++) {
        t1 = (i / (RealType)CIRCLE_RES) * M_PI * 2;
        t2 = ((i + 1) / (RealType)CIRCLE_RES) * M_PI * 2;
        Tmp1.x = Centre.x + Rad.x * cos(t1);
        Tmp1.y = Centre.y + Rad.x * sin(t1);

        Tmp2.x = Centre.x + Rad.x * cos(t2);
        Tmp2.y = Centre.y + Rad.x * sin(t2);
        SDL_RenderDrawLine(RendererInstance, Tmp1.x, Tmp1.y, Tmp2.x, Tmp2.y);
    }
}

void RendererFillCircle(PointStruct Centre, RealType Radius)
{
    IntType i;
    RealType MinX, MaxX, y;
    PointStruct 
        Rad = POINT(Radius, Radius);

    Centre = TRANS_APPLY_POINT(Transform, Centre);
    Rad = TRANS_APPLY_VEC(Transform, Rad);
    SDL_SetRenderDrawColor(RendererInstance, Foreground.r * 255,
                                             Foreground.g * 255,
                                             Foreground.b * 255,
                                             Foreground.a * 255);
    for (i = -Rad.x; i <= Rad.x; i++) {
        MaxX = Centre.x + sqrtf(Rad.x * Rad.x - i * i);
        MinX = Centre.x - sqrtf(Rad.x * Rad.x - i * i);
        y = Centre.y + i;
        SDL_RenderDrawLine(RendererInstance, MinX, y, MaxX, y);
    }
}

void RendererDrawRectangle(PointStruct TopLeft, PointStruct BottomRight)
{
    SDL_Rect Rect;

    TopLeft = TRANS_APPLY_POINT(Transform, TopLeft);
    BottomRight = TRANS_APPLY_POINT(Transform, BottomRight);
    Rect.x = TopLeft.x;
    Rect.y = TopLeft.y;
    Rect.w = BottomRight.x - TopLeft.x;
    Rect.h = BottomRight.y - TopLeft.y;
    SDL_SetRenderDrawColor(RendererInstance, Foreground.r * 255,
                                             Foreground.g * 255,
                                             Foreground.b * 255,
                                             Foreground.a * 255);
    SDL_RenderDrawRect(RendererInstance, &Rect);
}

void RendererFillRectangle(PointStruct TopLeft, PointStruct BottomRight)
{
    SDL_Rect Rect;

    TopLeft = TRANS_APPLY_POINT(Transform, TopLeft);
    BottomRight = TRANS_APPLY_POINT(Transform, BottomRight);
    Rect.x = TopLeft.x;
    Rect.y = TopLeft.y;
    Rect.w = BottomRight.x - TopLeft.x;
    Rect.h = BottomRight.y - TopLeft.y;
    SDL_SetRenderDrawColor(RendererInstance, Foreground.r * 255,
                                             Foreground.g * 255,
                                             Foreground.b * 255,
                                             Foreground.a * 255);
    SDL_RenderFillRect(RendererInstance, &Rect);
}

PointStruct RendererScreenToWorld(PointStruct ScreenPos)
{
    return TRANS_APPLY_POINT(TransformInverse, ScreenPos);
}

PointStruct RendererWorldToScreen(PointStruct WorldPos)
{
    return TRANS_APPLY_POINT(Transform, WorldPos);
}

void RendererDrawText(const char *Text, PointStruct Pos)
{
    SDL_Surface 
        *Surface = NULL;
    SDL_Texture
        *Texture = NULL;
    SDL_Color c;
    SDL_Rect SrcRect, DestRect;

    Pos = TRANS_APPLY_POINT(Transform, Pos);
    c.r = Foreground.r * 255;
    c.g = Foreground.g * 255;
    c.b = Foreground.b * 255;
    c.a = Foreground.a * 255;

    Surface = TTF_RenderText_Solid(Font, Text, c);
    Texture = SDL_CreateTextureFromSurface(RendererInstance, Surface);

    SrcRect.x = 0;
    SrcRect.y = 0;
    SrcRect.w = Surface -> w;
    SrcRect.h = Surface -> h;

    DestRect.x = Pos.x;
    DestRect.y = Pos.y;
    DestRect.w = Surface -> w;
    DestRect.h = Surface -> h;

    SDL_RenderCopy(RendererInstance, Texture, &SrcRect, &DestRect);
    SDL_DestroyTexture(Texture);
    SDL_FreeSurface(Surface);
}
