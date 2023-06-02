/******************************************************************************\
*  proton.h                                                                    *
*                                                                              *
*  Contains basic project definitions.                                         *
*                                                                              *
*              Written by Abed Na'ran                          May 2023        *
*                                                                              *
\******************************************************************************/
#ifndef CONFIG_H 
#define CONFIG_H 
#include <stdio.h>
#include <stdarg.h>
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
#define POINT(a, b) (PointStruct){.x = (a), .y = (b)}
/* Assume user enters an integer between 0 and 255.                           */
#define COLOUR(x, y, z, w) (ColourStruct){.r = ((x) / 255.f), \
                                          .g = ((y) / 255.f), \
                                          .b = ((z) / 255.f), \
                                          .a = ((w) / 255.f)}
#define PROJ "PROTON"
#define SPLASH "WRITTEN BY ABED NA'ARAN, COMPILED ON " __DATE__
#define BUFF_SIZE (1024)
#define BUFF_SIZE_SMALL (32)
#define TICKS_PER_SEC (60)
#define AABB_TOTAL (64)
#define MISC_TIMER_TOTAL (32)
#define MAX_EVENTS_PER_TICK (32)
#define KEYS_TOTAL (256)
#define MOUSE_KEYS_TOTAL (16)
#define M1 (SDL_BUTTON_LEFT)
#define M2 (SDL_BUTTON_RIGHT)
#define M3 (SDL_BUTTON_MIDDLE)
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define TIMER(NUM) \
        (TimerTicks[NUM] == INACTIVE ? INACTIVE : Tick - TimerTicks[NUM])
#define GAME_STATE_TIMER(STATE) \
        (GameStateTicks[STATE] == INACTIVE ? INACTIVE : Tick - GameStateTicks[STATE])
#define AABB_HIGHLIGHT_TIMER(NUM) \
        (AABBHighlightTicks[NUM] == INACTIVE ? INACTIVE : Tick - AABBHighlightTicks[STATE])
#define AABB_CLICK_TIMER(NUM) \
        (AABBClickTicks[NUM] == INACTIVE ? INACTIVE : Tick - AABBClickTicks[STATE])
#define ANIM_PARAM(TIMER, TICKS) \
        MIN(MAX((TIMER) / (TICKS), 0.f), 1.f) 
#define ANIM(T, START, END) \
        ((START) * (1.f - (T)) + (END) * (T))

#define AABB_SET(AABB, P1, P2)                                 \
    do {                                                       \
    PointStruct                                                \
        _1 = RendererWorldToScreen(P1),                        \
        _2 = RendererWorldToScreen(P2);                        \
    AABB[0] = POINT(MIN((_1).x, (_2).x), MIN((_1).y, (_2).y)); \
    AABB[1] = POINT(MAX((_1).x, (_2).x), MAX((_1).y, (_2).y)); \
    } while (FALSE);
#define AABB_IS_POINT_INSIDE(AABB, P)                          \
          ((P).x >= (AABB)[0].x && (P).x <= (AABB)[1].x && \
           (P).y >= (AABB)[0].y && (P).y <= (AABB)[1].y)
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
/* ORDER MATTERS, IT DETERMINES DRAWING/LOGIC ORDER.                          */
typedef enum {
    GAME_STATE_START = 0,
    GAME_STATE_CURVES,
    GAME_STATE_FILE_PICKER,
    GAME_STATE_MENU,
    GAME_STATE_TOTAL
} GameStateEnum;
typedef enum {
    AABB_TYPE_MENU_ITEM_0 = 0,
    AABB_TYPE_MENU_ITEM_1,
    AABB_TYPE_MENU_ITEM_2,
    AABB_TYPE_MENU_ITEM_3,
    AABB_TYPE_MENU_ITEM_4,
    AABB_TYPE_MENU_ITEM_5,
    AABB_TYPE_MENU_ITEM_6,
    AABB_TYPE_MENU_ITEM_7,
    AABB_TYPE_MENU_COUNT = 32,
} AABBTypeEnum;

extern PointStruct Mouse;
extern PointStruct LastMouse;
extern AABBType AABB[AABB_TOTAL];
extern IntType MouseWheel;
extern IntType LastMouseWheel;
extern BoolType GameState[GAME_STATE_TOTAL];
extern BoolType Timer[MISC_TIMER_TOTAL];
extern IntType GameStateTicks[GAME_STATE_TOTAL];
extern IntType TimerTicks[MISC_TIMER_TOTAL];
extern IntType AABBHighlightTicks[AABB_TOTAL];
extern IntType AABBClickTicks[AABB_TOTAL];
extern IntType Tick;
extern BoolType Keys[KEYS_TOTAL];
extern BoolType MouseKeys[MOUSE_KEYS_TOTAL];
extern BoolType LastKeys[KEYS_TOTAL];
extern BoolType LastMouseKeys[MOUSE_KEYS_TOTAL];
extern BoolType CanRun;
extern BoolType Debug;

static inline void LogInfo(char *Format, ...)
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

static inline void LogError(char *Format, ...)
{
    char Buff[BUFF_SIZE];
    va_list list;

    sprintf(Buff, "<%s:ERROR:> %s\n", PROJ, Format);
    va_start(list, Format);
    vfprintf(stderr, Buff, list);
    va_end(list);
    fflush(stderr);
}
#endif /* CONFIG_H */
