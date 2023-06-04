#ifndef PROTON_ENGINE_H
#define PROTON_ENGINE_H
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#define EPS (1e-4)
#define TESSELATION_RES (100)
#define TRUE (1)
#define FALSE (0)
#define RESET (2)
#define START (1)
#define STOP (0)
#define WIN_WIDTH (1280)
#define WIN_HEIGHT (720)
#define WIN_CENTRE POINT(WIN_WIDTH / 2.f, WIN_HEIGHT / 2.f)
#define POINT_ZERO POINT(0, 0)
#define POINT(a, b) \
        (PointStruct){.x = (a), .y = (b)}
#define POINT_ADD(P1, P2) \
        POINT((P1).x + (P2).x, (P1).y + (P2).y)
#define POINT_SCALE(P, S) \
        POINT((P).x * S, (P).y * S)
/* Assume user enters an integer between 0 and 255.                           */
#define COLOUR(x, y, z, w) \
        (ColourStruct){.r = ((x) / 255.f), \
                       .g = ((y) / 255.f), \
                       .b = ((z) / 255.f), \
                       .a = ((w) / 255.f)}
#define PROJ "PROTON"
#define SPLASH "WRITTEN BY ABED NA'ARAN, COMPILED ON " __DATE__
#define BUFF_SIZE (1024)
#define BUFF_SIZE_SMALL (32)
#define TICKS_PER_SEC (60)
#define MAX_EVENTS_PER_TICK (32)
#define KEYS_TOTAL (256)
#define MOUSE_KEYS_TOTAL (16)
#define M1 (SDL_BUTTON_LEFT)
#define M2 (SDL_BUTTON_RIGHT)
#define M3 (SDL_BUTTON_MIDDLE)
#define MIN(a, b) \
    ((a) <= (b) ? (a) : (b))
#define MAX(a, b) \
    ((a) >= (b) ? (a) : (b))
// #define AABB_SET(AABB, P1, P2)                                 \
//     do {                                                       \
//     PointStruct                                                \
//         _1 = ProtonWorldToScreen(POINT((IntType)(P1).x,      \
//                                          (IntType)(P1).y)),    \
//         _2 = ProtonWorldToScreen(POINT((IntType)(P2).x,      \
//                                          (IntType)(P2).y));    \
//     AABB[0] = POINT(MIN((_1).x, (_2).x), MIN((_1).y, (_2).y)); \
//     AABB[1] = POINT(MAX((_1).x, (_2).x), MAX((_1).y, (_2).y)); \
//     } while (FALSE);
#define AABB_SET(NUM, P1, P2)                                       \
    do {                                                            \
    PointStruct                                                     \
        _1 = ProtonWorldToScreen(P1),                               \
        _2 = ProtonWorldToScreen(P2);                               \
    AABB[NUM][0] = POINT(MIN((_1).x, (_2).x), MIN((_1).y, (_2).y)); \
    AABB[NUM][1] = POINT(MAX((_1).x, (_2).x), MAX((_1).y, (_2).y)); \
    TimersStartPoints[AABB_HOVER + NUM] = INACTIVE;                 \
    TimersStartPoints[AABB_NHOVER + NUM] = Tick;                    \
    } while (FALSE);
#define AABB_IS_POINT_INSIDE(AABB, P)                          \
          ((P).x >= (AABB)[0].x && (P).x <= (AABB)[1].x && \
           (P).y >= (AABB)[0].y && (P).y <= (AABB)[1].y)
#define AABB_COLLIDES(B1, B2) \
          (AABB[B1][0].x < AABB[B2][1].x && \
           AABB[B1][0].y < AABB[B2][1].y && \
           AABB[B2][0].x < AABB[B1][1].x && \
           AABB[B2][0].y < AABB[B1][1].y)

#define INACTIVE  (-1)
typedef float RealType;
typedef int IntType;
typedef unsigned char BoolType;
typedef RealType MatType[2][3];
typedef struct {
    RealType x, y; 
} PointStruct;
typedef PointStruct AABBType[2];
typedef struct {
    RealType r, g, b, a; /* Values are kept in [0, 1].                        */
} ColourStruct;
/* Timer things.                                                              */
/* General purpose timers.                                                    */
#define GP_TIMER_TOTAL (8)
#define GS_TOTAL (GS_END - GS_START + 1)
/* ORDER MATTERS, IT DETERMINES DRAWING/LOGIC ORDER.                          */
typedef enum {
    AABB_MENU_ITEM_0 = 0,
    AABB_MENU_ITEM_1,
    AABB_MENU_ITEM_2,
    AABB_MENU_ITEM_3,
    AABB_MENU_ITEM_4,
    AABB_MENU_ITEM_5,
    AABB_MENU_ITEM_6,
    AABB_MENU_ITEM_7,
    AABB_MENU_TOTAL = 32,
    AABB_TOTAL = 512,
} AABBEnum;
typedef enum {
    GP_0 = 0,
    GP_1,
    GP_2,
    GP_3,
    GP_4,
    GP_5,
    GP_6,
    GP_7,
    GP_END = GP_TIMER_TOTAL,
    GS_START,
    GS_CURVES,
    GS_FILE_PICKER,
    GS_MENU,
    GS_END,
    TIMER_CONTROLLED_TOTAL, /* All the controllable timers.                   */
    AABB_HOVER, /* All the timers managed by proton.                          */
    AABB_HOVER_END = AABB_HOVER + AABB_TOTAL,
    AABB_NHOVER,
    AABB_NHOVER_END = AABB_NHOVER + AABB_TOTAL,
    AABB_CLICK,
    AABB_CLICK_END = AABB_CLICK + AABB_TOTAL,
    AABB_NCLICK,
    AABB_NCLICK_END = AABB_NCLICK + AABB_TOTAL,
    TIMER_TOTAL,
} TimersEnum;
#define TIMER_RESET(NUM) \
        (TimerControllers[NUM] = 2)
#define TIMER_START(NUM) \
        (TimerControllers[NUM] = 1)
#define TIMER_STOP(NUM) \
        (TimerControllers[NUM] = 0)
#define TIMER(NUM) \
        (TimersStartPoints[NUM] == INACTIVE ? INACTIVE : Tick - TimersStartPoints[NUM])
#define ANIM_PARAM(TIMER, TICKS) \
        MIN(MAX((TIMER) / ((RealType)TICKS), 0.f), 1.f) 
#define ANIM(T, START, END) \
        ((START) * (1.f - (T)) + (END) * (T))

extern AABBType AABB[AABB_TOTAL];
extern PointStruct Mouse;
extern PointStruct LastMouse;
extern IntType MouseWheel;
extern IntType LastMouseWheel;
extern IntType TimersStartPoints[TIMER_TOTAL];
extern IntType Tick;
extern BoolType TimerControllers[TIMER_CONTROLLED_TOTAL];
extern BoolType Keys[KEYS_TOTAL];
extern BoolType Buttons[MOUSE_KEYS_TOTAL];
extern BoolType LastKeys[KEYS_TOTAL];
extern BoolType LastButtons[MOUSE_KEYS_TOTAL];
extern BoolType CanRun;
extern BoolType Debug;

static inline void ProtonLogInfo(char *Format, ...)
{
    char Buff[BUFF_SIZE];
    va_list list;

    sprintf(Buff, "<%s> %s\n", PROJ, Format);
    va_start(list, Format);
    vfprintf(stdout, Buff, list);
    va_end(list);
    fflush(stdout);
}

static inline void LogWarn(char *Format, ...)
{
    char Buff[BUFF_SIZE];
    va_list list;

    sprintf(Buff, "<%s:WARNING:> %s\n", PROJ, Format);
    va_start(list, Format);
    vfprintf(stderr, Buff, list);
    va_end(list);
    fflush(stderr);
}

static inline void ProtonLogError(char *Format, ...)
{
    char Buff[BUFF_SIZE];
    va_list list;

    sprintf(Buff, "<%s:ERROR:> %s\n", PROJ, Format);
    va_start(list, Format);
    vfprintf(stderr, Buff, list);
    va_end(list);
    fflush(stderr);
}

static inline void AABBResolveCollision(IntType B1, BoolType B1Static,
                                        IntType B2, BoolType B2Static)
{
    RealType x12, x21, y12, y21, Min, a, b;

    if (!AABB_COLLIDES(B1, B2))
        return;

    if (B1Static && B2Static)
        return;

    x12 = AABB[B2][1].x - AABB[B1][0].x;
    x21 = AABB[B1][1].x - AABB[B2][0].x;
    y12 = AABB[B2][1].y - AABB[B1][0].y;
    y21 = AABB[B1][1].y - AABB[B2][0].y;
    Min = MIN(x12, MIN(x21, MIN(y12, y21)));

    if (B1Static) {
        a = Min == x12 || Min == y12 ? 0 : Min;
        b = Min == x12 || Min == y12 ? Min : 0;
    } else if (B2Static) {
        a = Min == x12 || Min == y12 ? Min : 0;
        b = Min == x12 || Min == y12 ? 0 : Min;
    } else
        a = b = Min / 2.f;

    if (Min == x12) {
        AABB[B1][0].x += a;
        AABB[B1][1].x += a;
        AABB[B2][0].x -= b;
        AABB[B2][1].x -= b;
    } else if (Min == y12) {
        AABB[B1][0].y += a;
        AABB[B1][1].y += a;
        AABB[B2][0].y -= b;
        AABB[B2][1].y -= b;
    } else if (Min == x21) {
        AABB[B2][0].x += a;
        AABB[B2][1].x += a;
        AABB[B1][0].x -= b;
        AABB[B1][1].x -= b;
    } else { 
        AABB[B2][0].y += a;
        AABB[B2][1].y += a;
        AABB[B1][0].y -= b;
        AABB[B1][1].y -= b;
    }
}

int ProtonEngineRun(int argc, const char *argv[]);
#endif /* PROTON_ENGINE_H */
