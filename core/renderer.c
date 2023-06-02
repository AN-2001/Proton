#include <SDL2/SDL.h>
#include "SDL2/SDL_ttf.h"
#include <math.h>
#include <memory.h>
#include <stdint.h>
#include <string.h>
#include "renderer.h"
#include "proton.h"

#define CIRCLE_RES (32)
#define TRANSFORM_STACK_SIZE (32)
#define TRANS_APPLY_POINT(T, P) TransformApply(T, P, TRUE)
#define TRANS_APPLY_VEC(T, V) TransformApply(T, V, FALSE)

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
    TransformStack[TRANSFORM_STACK_SIZE],
    InverseTransformStack[TRANSFORM_STACK_SIZE];
const static MatType 
        Identity = {
            {1.f, 0.f, 0.f},
            {0.f, 1.f, 0.f},
        };
static MatType Transform, InverseTransform;
static TTF_Font
    *Font = NULL;
    
static inline PointStruct TransformApply(MatType Transform,
                                  PointStruct Point,
                                  BoolType IsPoint)
{
    return POINT(Point.x * Transform[0][0] +
                 Point.y * Transform[0][1] +
                 Transform[0][2] * IsPoint,

                 Point.x * Transform[1][0] +
                 Point.y * Transform[1][1] +
                 Transform[1][2] * IsPoint);
}

static inline void TransformMult(MatType Left, MatType Right, MatType Dest)
{
    MatType Ret;

    Ret[0][0] = Left[0][0] * Right[0][0] + Left[0][1] * Right[1][0];
    Ret[1][0] = Left[1][0] * Right[0][0] + Left[1][1] * Right[1][0]; 

    Ret[0][1] = Left[0][0] * Right[0][1] + Left[0][1] * Right[1][1];
    Ret[1][1] = Left[1][0] * Right[0][1] + Left[1][1] * Right[1][1]; 

    Ret[0][2] = Left[0][0] * Right[0][2] + Left[0][1] * Right[1][2] + Left[0][2];
    Ret[1][2] = Left[1][0] * Right[0][2] + Left[1][1] * Right[1][2] + Left[1][2]; 

    memcpy(Dest, Ret, sizeof(MatType));
}

static inline void TransformInvert(MatType Transform, MatType Inverse)
{
    RealType
        Det = Transform[0][0] * Transform[1][1] -
              Transform[0][1] * Transform[1][0];

    Inverse[0][0] =  Transform[0][0] / Det;
    Inverse[1][0] =  Transform[0][1] / Det;
    Inverse[0][1] =  Transform[1][0] / Det;
    Inverse[1][1] =  Transform[1][1] / Det;
    
    Inverse[0][2] = Inverse[0][0] * -Transform[0][2] +
                    Inverse[0][1] * -Transform[1][2] ;

    Inverse[1][2] = Inverse[1][0] * -Transform[0][2] +
                    Inverse[1][1] * -Transform[1][2];

}

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
    memcpy(InverseTransform, Identity, sizeof(InverseTransform));
}

void RendererFree()
{
    TTF_CloseFont(Font);
}

void RendererPushTransform()
{
    TransformTop = MIN(TransformTop + 1, TRANSFORM_STACK_SIZE - 1);

    memcpy(TransformStack[TransformTop],
           Transform,
           sizeof(Transform));

    memcpy(InverseTransformStack[TransformTop],
           InverseTransform,
           sizeof(InverseTransform));
}

void RendererTranslate(PointStruct Point)
{
    MatType
        Trans;

    memcpy(Trans, Identity, sizeof(Trans));
    Trans[0][2] = Point.x;
    Trans[1][2] = Point.y;

    TransformMult(Trans, Transform, Transform);
    TransformInvert(Transform, InverseTransform);
}

void RendererScale(RealType Scale)
{
    MatType
        Trans;

    memcpy(Trans, Identity, sizeof(Trans));
    Trans[0][0] = Scale;
    Trans[1][1] = Scale;

    TransformMult(Trans, Transform, Transform);
    TransformInvert(Transform, InverseTransform);
}

void RendererRotate(RealType Ang)
{
    MatType
        Trans;
    RealType
        Cos = cosf(Ang),
        Sin = sinf(Ang);

    memcpy(Trans, Identity, sizeof(Trans));
    Trans[0][0] = Cos;
    Trans[0][1] = -Sin;
    Trans[1][0] = Sin;
    Trans[1][1] = Cos;

    TransformMult(Trans, Transform, Transform);
    TransformInvert(Transform, InverseTransform);
}

void RendererPopTransform()
{
    if (TransformTop == -1)
        return;

    memcpy(Transform,
           TransformStack[TransformTop],
           sizeof(Transform));

    memcpy(InverseTransform,
           InverseTransformStack[TransformTop],
           sizeof(InverseTransform));

    TransformTop = MAX(TransformTop - 1, -1);
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

void RendererSetFG(IntType r, IntType g, IntType b)
{
    Foreground = COLOUR(r, g, b, 255);
}

void RendererSetBG(IntType r, IntType g, IntType b)
{
    Background = COLOUR(r, g, b, 255);
}

void RendererSetFGA(IntType r, IntType g, IntType b, IntType a)
{
    Foreground = COLOUR(r, g, b, a);
}

void RendererSetBGA(IntType r, IntType g, IntType b, IntType a)
{
    Background = COLOUR(r, g, b, a);
}

void RendererDrawPoint(PointStruct Point)
{
    Point = TRANS_APPLY_POINT(Transform, Point);
    if (Point.x < 0 || Point.x >= WIN_WIDTH ||
        Point.y < 0 || Point.y >= WIN_HEIGHT)
        return;

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

    if ((P1.x < 0 || P1.x >= WIN_WIDTH ||
        P1.y < 0 || P1.y >= WIN_HEIGHT) &&
        (P2.x < 0 || P2.x >= WIN_WIDTH ||
        P2.y < 0 || P2.y >= WIN_HEIGHT))
        return;

    SDL_SetRenderDrawColor(RendererInstance, Foreground.r * 255,
                                             Foreground.g * 255,
                                             Foreground.b * 255,
                                             Foreground.a * 255);
    SDL_RenderDrawLine(RendererInstance, P1.x, P1.y, P2.x, P2.y);
}

void RendererDrawCircle(PointStruct Centre, RealType Radius)
{
    IntType i;
    RealType t1, t2, S;
    PointStruct Tmp1, Tmp2;

    Centre = TRANS_APPLY_POINT(Transform, Centre);
    S = sqrt(Transform[0][0] * Transform[1][1] - 
             Transform[0][1] * Transform[1][0]);
    Radius = Radius * S;

    SDL_SetRenderDrawColor(RendererInstance, Foreground.r * 255,
                                             Foreground.g * 255,
                                             Foreground.b * 255,
                                             Foreground.a * 255);
    for (i = 0; i <= CIRCLE_RES; i++) {
        t1 = (i / (RealType)CIRCLE_RES) * M_PI * 2;
        t2 = ((i + 1) / (RealType)CIRCLE_RES) * M_PI * 2;
        Tmp1.x = Centre.x + Radius * cos(t1);
        Tmp1.y = Centre.y + Radius * sin(t1);

        Tmp2.x = Centre.x + Radius * cos(t2);
        Tmp2.y = Centre.y + Radius * sin(t2);
        SDL_RenderDrawLine(RendererInstance, Tmp1.x, Tmp1.y, Tmp2.x, Tmp2.y);
    }
}

void RendererFillCircle(PointStruct Centre, RealType Radius)
{
    IntType i, y, Start, End;
    RealType R, S, v;

    Centre = TRANS_APPLY_POINT(Transform, Centre);
    S = sqrt(Transform[0][0] * Transform[1][1] - 
             Transform[0][1] * Transform[1][0]);
    Radius = Radius * S;
    Start = Centre.y - Radius;
    End = Centre.y + Radius;
    if (Start >= WIN_HEIGHT || End < 0)
        return;

    if (Start < 0)
        Start = -Centre.y;
    else
        Start = -Radius;

    if (End >= WIN_HEIGHT)
        End = WIN_HEIGHT - 1 - Centre.y;
    else
        End = Radius;
    R = Radius * Radius;
    SDL_SetRenderDrawColor(RendererInstance, Foreground.r * 255,
                                             Foreground.g * 255,
                                             Foreground.b * 255,
                                             Foreground.a * 255);
    for (i = Start; i <= End; i++) {
        y = Centre.y + i;
        v = sqrt(R - i * i);
        SDL_RenderDrawLine(RendererInstance,
                           MAX(Centre.x - v, 0),
                           y,
                           MIN(Centre.x + v, WIN_WIDTH),
                           y);
    }
}

void RendererDrawRectangle(PointStruct TopLeft, PointStruct BottomRight)
{
    SDL_Rect Rect;

    TopLeft = TRANS_APPLY_POINT(Transform, TopLeft);
    BottomRight = TRANS_APPLY_POINT(Transform, BottomRight);

    SDL_SetRenderDrawColor(RendererInstance, Foreground.r * 255,
                                             Foreground.g * 255,
                                             Foreground.b * 255,
                                             Foreground.a * 255);
    SDL_RenderDrawLine(RendererInstance,
                       TopLeft.x,
                       TopLeft.y,
                       BottomRight.x,
                       TopLeft.y);

    SDL_RenderDrawLine(RendererInstance,
                       BottomRight.x,
                       TopLeft.y,
                       BottomRight.x,
                       BottomRight.y);

    SDL_RenderDrawLine(RendererInstance,
                       BottomRight.x,
                       BottomRight.y,
                       TopLeft.x,
                       BottomRight.y);

    SDL_RenderDrawLine(RendererInstance,
                       TopLeft.x,
                       BottomRight.y,
                       TopLeft.x,
                       TopLeft.y);

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
    return TRANS_APPLY_POINT(InverseTransform, ScreenPos);
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
