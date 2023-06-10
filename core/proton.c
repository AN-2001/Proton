#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "feel.h"
#include "proton.h"
#include "renderer.h"
#include "curves.h"
#include "menu.h"
#include "filePicker.h"

#define EVENT_HEADER(i) (void)Event;
#define MSEC_PER_TICK ((1.0 / TICKS_PER_SEC) * 1e3)
#define INTRO_LEN (100.f)

static SDL_Window *MainWindow;
static SDL_Renderer *MainRenderer;
static void PopulateEventQueue();
static void HandleQuitEvent(SDL_Event Event);
static void HandleKeyboardEvent(SDL_Event Event);
static void HandleMouseMotionEvent(SDL_Event Event);
static void HandleMousePressedEvent(SDL_Event Event);
static void HandleMouseWheelEvent(SDL_Event Event);
static void HandleRender();
static void DoStart(RealType DeltaTime);
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
    Buttons[MOUSE_KEYS_TOTAL] =
        {[ 0 ... MOUSE_KEYS_TOTAL - 1 ] = 0},
    LastButtons[MOUSE_KEYS_TOTAL] =
        {[ 0 ... MOUSE_KEYS_TOTAL - 1 ] = 0},
    TimerControllers[TIMER_CONTROLLED_TOTAL] =
        {[ 0 ... TIMER_CONTROLLED_TOTAL - 1 ] = 0},
    LastTimerControllers[TIMER_CONTROLLED_TOTAL] =
        {[ 0 ... TIMER_CONTROLLED_TOTAL - 1 ] = 0},
    CanRun = TRUE,
    Debug = FALSE;
IntType
    TimersStartPoints[TIMER_TOTAL] =
            {[ 0 ... TIMER_TOTAL - 1] = INACTIVE},
    Tick = INACTIVE,
    MouseWheel = 0,
    LastMouseWheel = 0;

/* Add new states here.                                                       */
struct {
    char Name[BUFF_SIZE_SMALL];
    void (*Update)(RealType Delta);
    void (*Draw)();
} StateInfo[GS_TOTAL] = {
    {"Start", DoStart, DrawStart}, /* The state below will run after start.   */
    {"Curves", CurvesUpdate, CurvesDraw},
};

int main(int argc, const char *argv[])
{
    SDL_Event CurrentEvent;
    IntType DT, i, j, NumEvents, CT, LT, DeltaTime;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
        ProtonLogError("COULDN'T START SDL: %s.", SDL_GetError());
        return 1;
    }

    MainWindow = SDL_CreateWindow(PROJ,
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  WIN_WIDTH,
                                  WIN_HEIGHT,
                                  SDL_WINDOW_ALWAYS_ON_TOP);

    if (!MainWindow) {
        ProtonLogError("COULDN'T CREATE THE MAIN WINDOW: %s.", SDL_GetError());
        return 1;
    }

    MainRenderer = SDL_CreateRenderer(MainWindow,
                                      -1,
                                      SDL_RENDERER_ACCELERATED);
    if (!MainRenderer) {
        ProtonLogError("COULDN'T CREATE THE MAIN RENDERER: %s.", SDL_GetError());
        return (SDL_DestroyWindow(MainWindow), 1);
    }

    RendererBind(MainRenderer, HandleRender);
    RendererInit();
    TIMER_START(GS_START);
    ProtonLogInfo(SPLASH);
    ProtonLogInfo("Initialized.");

    CT = SDL_GetTicks();
    LT = SDL_GetTicks();
    DeltaTime = 0;
    while (CanRun) {
        /* Do nothing if we're out of focus to conserve CPU.                  */
        if (!(SDL_GetWindowFlags(MainWindow) & SDL_WINDOW_MOUSE_FOCUS))
            SDL_WaitEvent(NULL);

        /* First tick should run instantly.                                   */
        if (Tick != INACTIVE) {
            (CT = SDL_GetTicks(), DT = CT - LT, LT = CT);
            DeltaTime += DT;
            if (DeltaTime < MSEC_PER_TICK) {
                /* Sleep a bit to not waste CPU usage.                        */
                SDL_Delay((IntType)((MSEC_PER_TICK - DeltaTime) / 2.f));
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
        
        /* Update collision.                                                  */
        for (i = 0; i < AABB_TOTAL; i++) {
            if (AABB_IS_POINT_INSIDE(AABB[i], Mouse)) {
                TimersStartPoints[AABB_NHOVER + i] = INACTIVE;

                if (TimersStartPoints[AABB_HOVER + i] == INACTIVE)
                    TimersStartPoints[AABB_HOVER + i] = Tick;

                if (Buttons[M1]) {
                    TimersStartPoints[AABB_NCLICK + i] = INACTIVE;
                    if (TimersStartPoints[AABB_CLICK + i] == INACTIVE) 
                        TimersStartPoints[AABB_CLICK + i] = Tick;
                } else {
                    TimersStartPoints[AABB_CLICK + i] = INACTIVE;
                    if (TimersStartPoints[AABB_NCLICK + i] == INACTIVE)
                        TimersStartPoints[AABB_NCLICK + i] = Tick;
                }

            } else {
                TimersStartPoints[AABB_CLICK + i] = INACTIVE;
                TimersStartPoints[AABB_HOVER + i] = INACTIVE;
                if (TimersStartPoints[AABB_NCLICK + i] == INACTIVE)
                    TimersStartPoints[AABB_NCLICK + i] = Tick;
                if (TimersStartPoints[AABB_NHOVER + i] == INACTIVE)
                    TimersStartPoints[AABB_NHOVER + i] = Tick;
            }

        }

        /* Update game logic.                                                 */
        for (i = 0; i < TIMER_CONTROLLED_TOTAL; i++) {
            if (TimerControllers[i] == RESET) {
                TimersStartPoints[i] = Tick;    
                TimerControllers[i] = START;
                LastTimerControllers[i] = TimerControllers[i];
            } else if (TimerControllers[i] != LastTimerControllers[i]) {
                TimersStartPoints[i] = TimerControllers[i] ? Tick : INACTIVE; 
                LastTimerControllers[i] = TimerControllers[i];
            }
        }

        for (i = 0; i < GS_TOTAL; i++)
            if (StateInfo[i].Update &&
                    TimersStartPoints[GS_START + i] != INACTIVE)
                StateInfo[i].Update(((RealType)DeltaTime) * 1e-3);

        memcpy(&LastMouse, &Mouse, sizeof(Mouse));
        memcpy(&LastMouseWheel, &LastMouseWheel, sizeof(MouseWheel));
        memcpy(LastKeys, Keys, sizeof(Keys));
        memcpy(LastButtons, Buttons, sizeof(Buttons));

        ProtonRenderFrame();
        /* Reset the frame capper.                                            */
        DeltaTime = 0;
    }
   
    RendererFree();
    SDL_DestroyRenderer(MainRenderer);
    SDL_DestroyWindow(MainWindow);
    SDL_Quit();
    ProtonLogInfo("Done!");
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

    Buttons[Event.button.button] = Event.type == SDL_MOUSEBUTTONDOWN;
}

void HandleMouseWheelEvent(SDL_Event Event)
{
    EVENT_HEADER(4);

    MouseWheel = Event.wheel.y;
}

void HandleRender()
{
    IntType i;

    for (i = 0; i < GS_TOTAL; i++)
        if (StateInfo[i].Draw && 
                TimersStartPoints[GS_START + i] != INACTIVE)
            StateInfo[i].Draw();

}

void DoStart(RealType DeltaTime)
{
    (void)DeltaTime;

   if (TIMER(GS_START) == INTRO_LEN) {
        TIMER_STOP(GS_START);
        TIMER_START(GS_START + 1);
   }
}

void DrawStart()
{
    RealType t;


   t = ANIM_PARAM(TIMER(GS_START), INTRO_LEN);
   t = Casteljau1D(t, EaseIn);
   t = ANIM(t, 255, 0);

   ProtonSetBG(COLOUR(t, t, t, 255));
   ProtonSetFG(COLOUR(0, 0, 0, t));
   ProtonDrawText("PROTON", POINT(WIN_WIDTH / 2.f - 60, WIN_HEIGHT / 2.f));
}
