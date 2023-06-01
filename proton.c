/******************************************************************************\
*  proton.c                                                                     *
*                                                                              *
*  SDL Prototype project implements some things I learned from doom/quake      *
*  source code.                                                                *
*                                                                              *
*              Written by Abed Na'ran                          May 2023        *
*                                                                              *
\******************************************************************************/

#include <SDL2/SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "SDL_events.h"
#include "SDL_video.h"
#include "proton.h"
#include "renderer.h"
#include "curves.h"
#include "menu.h"
#include "filePicker.h"

#define EVENT_HEADER(i) (void)Event;
#define MSEC_PER_TICK ((1.0 / TICKS_PER_SEC) * 1e3)

static SDL_Window *MainWindow;
static SDL_Renderer *MainRenderer;
static void PopulateEventQueue();
static void HandleQuitEvent(SDL_Event Event);
static void HandleKeyboardEvent(SDL_Event Event);
static void HandleMouseMotionEvent(SDL_Event Event);
static void HandleMousePressedEvent(SDL_Event Event);
static void HandleMouseWheelEvent(SDL_Event Event);
static void HandleRender();
static void DoStart(IntType DeltaTime);
static void DrawStart();

PointStruct
    Mouse = POINT_ZERO,
    LastMouse = POINT_ZERO;
AABBType
    AABB[AABB_TOTAL] = 
        {[ 0 ... AABB_TOTAL - 1 ] = {POINT_ZERO, POINT_ZERO}};
BoolType
    Keys[KEYS_TOTAL] =
        {[ 0 ... KEYS_TOTAL - 1 ] = 0},
    LastKeys[KEYS_TOTAL] =
        {[ 0 ... KEYS_TOTAL - 1 ] = 0},
    MouseKeys[MOUSE_KEYS_TOTAL] =
        {[ 0 ... MOUSE_KEYS_TOTAL - 1 ] = 0},
    LastMouseKeys[MOUSE_KEYS_TOTAL] =
        {[ 0 ... MOUSE_KEYS_TOTAL - 1 ] = 0},
    GameState[GAME_STATE_TOTAL] = 
        {[ 0 ... GAME_STATE_TOTAL - 1 ] = 0},
    Timer[MISC_TIMER_TOTAL] = 
        {[ 0 ... MISC_TIMER_TOTAL - 1 ] = 0},
    LastGameState[GAME_STATE_TOTAL] = 
        {[ 0 ... GAME_STATE_TOTAL - 1 ] = 0},
    LastTimer[MISC_TIMER_TOTAL] = 
        {[ 0 ... MISC_TIMER_TOTAL - 1 ] = 0},
    CanRun = TRUE,
    Debug = FALSE;
IntType
    GameStateTicks[GAME_STATE_TOTAL] =
            {[ 0 ... GAME_STATE_TOTAL - 1] = INACTIVE},
    TimerTicks[MISC_TIMER_TOTAL] =
            {[ 0 ... MISC_TIMER_TOTAL - 1] = INACTIVE},
    AABBHighlightTicks[AABB_TOTAL] =
            {[ 0 ... AABB_TOTAL - 1] = INACTIVE},
    AABBClickTicks[AABB_TOTAL] =
            {[ 0 ... AABB_TOTAL - 1] = INACTIVE},
    Tick = INACTIVE,
    MouseWheel = 0,
    LastMouseWheel = 0;

struct {
    char Name[BUFF_SIZE_SMALL];
    void (*Update)(IntType Delta);
    void (*Draw)();
} StateInfo[GAME_STATE_TOTAL] = {
    {"Start", DoStart, DrawStart},
    {"Curves", CurvesUpdate, CurvesDraw},
    {"FilePicker", FilePickerUpdate, FilePickerDraw},
    {"Menu", MenuUpdate, MenuDraw},
};

int main()
{
    SDL_Event CurrentEvent;
    IntType DT, i, NumEvents, CT, LT, DeltaTime;

    if (SDL_Init(SDL_INIT_VIDEO)) {
        LogError("COULDN'T START SDL");
        return 1;
    }

    MainWindow = SDL_CreateWindow(PROJ,
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  WIN_WIDTH,
                                  WIN_HEIGHT,
                                  SDL_WINDOW_ALWAYS_ON_TOP);

    if (!MainWindow) {
        LogError("COULDN'T CREATE THE MAIN WINDOW");
        return 1;
    }

    MainRenderer = SDL_CreateRenderer(MainWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!MainRenderer) {
        LogError("COULDN'T CREATE THE MAIN RENDERER");
        return (SDL_DestroyWindow(MainWindow), 1);
    }

    RendererBind(MainRenderer, HandleRender);
    GameState[GAME_STATE_START] = TRUE;
    LogInfo(SPLASH);
    LogInfo("Initialized.\n");

    CT = SDL_GetTicks();
    LT = SDL_GetTicks();
    DeltaTime = 0;
    while (CanRun) {
        /* Do nothing if we're out of focus to conserve CPU.                  */
        if (!(SDL_GetWindowFlags(MainWindow) & SDL_WINDOW_INPUT_FOCUS)) {
            SDL_WaitEvent(NULL);
            CT = LT = SDL_GetTicks();
            DeltaTime = 0;
            continue;
        }

        /* First tick should run instantly.                                   */
        if (Tick != INACTIVE) {
            (CT = SDL_GetTicks(), DT = CT - LT, LT = CT);
            DeltaTime += DT;
            if (DeltaTime < MSEC_PER_TICK) {
                /* Sleep a bit to not waste CPU usage.                        */
                SDL_Delay((MSEC_PER_TICK - DeltaTime) / 2);
                continue;
            }
        }

        Tick++;
        MouseWheel = 0;
        NumEvents = 0;
        while (NumEvents < MAX_EVENTS_PER_TICK &&
                SDL_PollEvent(&CurrentEvent)) {
            switch (CurrentEvent.type) {
                case SDL_QUIT:
                    HandleQuitEvent(CurrentEvent);
                    break;
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    HandleKeyboardEvent(CurrentEvent);
                    break;
                case SDL_MOUSEMOTION:
                    HandleMouseMotionEvent(CurrentEvent);
                    break;
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEBUTTONDOWN:
                    HandleMousePressedEvent(CurrentEvent);
                    break;
                case SDL_MOUSEWHEEL:
                    HandleMouseWheelEvent(CurrentEvent);
                    break;
            }
        }
        
        /* Update collision checkers with mouse.                              */
        for (i = 0; i < AABB_TOTAL; i++) {
            if (AABB_IS_POINT_INSIDE(AABB[i], Mouse)) {
                if (AABBHighlightTicks[i] == INACTIVE)
                    AABBHighlightTicks[i] = Tick;

                if (MouseKeys[M1]) {
                    if (AABBClickTicks[i] == INACTIVE)
                        AABBClickTicks[i] = Tick;
                } else
                    AABBClickTicks[i] = INACTIVE;

            } else {
                AABBHighlightTicks[i] = INACTIVE;
                AABBClickTicks[i] = INACTIVE;
            }

        }

        /* Update game logic.                                                 */

        for (i = 0; i < MISC_TIMER_TOTAL; i++) {
            if (Timer[i] == RESET) {
                TimerTicks[i] = Tick;    
                Timer[i] = START;
                LastTimer[i] = Timer[i];
            } else if (Timer[i] != LastTimer[i]) {
                TimerTicks[i] = Timer[i] ? Tick : INACTIVE; 
                LastTimer[i] = Timer[i];
            }
        }

        for (i = 0; i < GAME_STATE_TOTAL; i++) {
            if (GameState[i] == RESET) {
                GameStateTicks[i] = Tick;    
                GameState[i] = START;
                LastGameState[i] = GameState[i];
            } else if (GameState[i] != LastGameState[i]) {
                GameStateTicks[i] = GameState[i] ? Tick : INACTIVE; 
                LastGameState[i] = GameState[i];
            }
        }

        for (i = 0; i < GAME_STATE_TOTAL; i++)
            if (StateInfo[i].Update &&
                    GameStateTicks[i] != INACTIVE)
                StateInfo[i].Update(DeltaTime);

        memcpy(&LastMouse, &Mouse, sizeof(Mouse));
        memcpy(&LastMouseWheel, &LastMouseWheel, sizeof(MouseWheel));
        memcpy(LastKeys, Keys, sizeof(Keys));
        memcpy(LastMouseKeys, MouseKeys, sizeof(MouseKeys));

        RendererRenderFrame();
        /* Reset the frame capper.                                            */
        DeltaTime = 0;
    }
   
    RendererFree();
    SDL_DestroyRenderer(MainRenderer);
    SDL_DestroyWindow(MainWindow);
    LogInfo("Done!\n");
    return 0;
}

void HandleQuitEvent(SDL_Event Event) 
{
   EVENT_HEADER(0);
   CanRun = FALSE;
}

void HandleKeyboardEvent(SDL_Event Event)
{
    EVENT_HEADER(1);
    IntType
        Sym = Event.key.keysym.sym;

    if (Sym >= 0 && Sym < KEYS_TOTAL)
        Keys[Sym] = (Event.type == SDL_KEYDOWN);
}

void HandleMouseMotionEvent(SDL_Event Event)
{
    EVENT_HEADER(2);

    Mouse = POINT(Event.motion.x, Event.motion.y);
}

void HandleMousePressedEvent(SDL_Event Event)
{
    EVENT_HEADER(3);

    MouseKeys[Event.button.button] = Event.type == SDL_MOUSEBUTTONDOWN;
}

void HandleMouseWheelEvent(SDL_Event Event)
{
    EVENT_HEADER(4);

    MouseWheel = Event.wheel.y;
}

void HandleRender()
{
    IntType i;

    for (i = 0; i < GAME_STATE_TOTAL; i++)
        if (StateInfo[i].Draw && 
                GameStateTicks[i] != INACTIVE)
            StateInfo[i].Draw();

    if (Debug) {
        RendererSetFGColour(0, 255, 0);
        for (i = 0; i < AABB_TOTAL; i++)
            RendererDrawRectangle(AABB[i][0], AABB[i][1]);
    }
}

void DoStart(IntType DeltaTime)
{
    (void)DeltaTime;
    IntType
        TickDelta = Tick - GameStateTicks[GAME_STATE_START];

    if (TickDelta == 100) {
        GameState[GAME_STATE_START] = FALSE;
        GameState[GAME_STATE_CURVES] = TRUE;
    }
}

void DrawStart()
{
    IntType
        TickDelta = Tick - GameStateTicks[GAME_STATE_START];
    RealType t;

    if (TickDelta == 0)
        RendererInit();

    t = TickDelta / 100.f;
    t = MIN(t, 1.f);

    RendererSetBGColour(255 * (1 - t), 255 * (1 - t), 255 * (1 - t));
    RendererSetFGColour(0, 0, 0);
    RendererDrawText("PROTON", POINT(WIN_WIDTH / 2.f - 60, WIN_HEIGHT / 2.f));
}
