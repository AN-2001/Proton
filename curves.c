#include "proton.h"
#include "renderer.h"
#include "SDL2/SDL.h"
#include "curves.h"
#include <math.h>
#include "feel.h"

#define CONTORL_PONTS_TOTAL (128)
#define GRID_SIZE_MAJOR (50)
#define GRID_SIZE_MINOR (GRID_SIZE_MAJOR / 2)
#define S_FACTOR (4.f)

static IntType
    CurrentPicked = INACTIVE;
static CurveStruct Curve;
static RealType
    Scale = 1.f;
static PointStruct 
    EvalCurve[TESSELATION_RES],
    Pos = POINT_ZERO;
static 
    BoolType IsEditing = FALSE;

static void DeleteControlPoint(IntType Index);
static void UpdateCurve();
static void UpdateAABB();

void CurvesUpdate(IntType DeltaTime)
{
    IntType i;
    IntType
        SnapAmount = GRID_SIZE_MINOR,
        XOffset = (int)((int)Pos.x / (WIN_WIDTH)) * (WIN_WIDTH),
        YOffset = (int)((int)Pos.y / (WIN_HEIGHT)) * (WIN_HEIGHT);
    PointStruct
        SnappedPosition = Mouse;

    if (Tick - GameStateTicks[GAME_STATE_CURVES] < 50)
        return;

    Scale *= exp(MouseWheel * DeltaTime * 1e-2);
    Scale = MAX(MIN(Scale, 10.f), 1 / 2.f);
    if (MouseKeys[M3]) {
        Pos.x = Pos.x - (Mouse.x - LastMouse.x) / Scale;
        Pos.y = Pos.y - (Mouse.y - LastMouse.y) / Scale;
    }

    if (Keys['s']) {
        if (Keys['a'])
            SnapAmount = GRID_SIZE_MAJOR;

        RendererPushTransform(POINT(-WIN_CENTRE.x * S_FACTOR + XOffset, 
                                    -WIN_CENTRE.y * S_FACTOR + YOffset), 1.f);
        RendererPushTransform(POINT(-Pos.x, -Pos.y), 1.f);
        RendererPushTransform(WIN_CENTRE, Scale);

        SnappedPosition = RendererScreenToWorld(SnappedPosition);
        SnappedPosition.x = roundf(SnappedPosition.x / SnapAmount) * SnapAmount;
        SnappedPosition.y = roundf(SnappedPosition.y / SnapAmount) * SnapAmount;
        SnappedPosition = RendererWorldToScreen(SnappedPosition);

        RendererPopTransform();
        RendererPopTransform();
        RendererPopTransform();
    }

    if (MouseKeys[M2] && !LastMouseKeys[M2])
        GameState[GAME_STATE_MENU] = TRUE;

    /* Set the origin to where Pos is.                                        */
    RendererPushTransform(POINT(-Pos.x, -Pos.y), 1.f);
    /* Set the zoom and move everything to the middle of the screen.          */
    RendererPushTransform(WIN_CENTRE, Scale);
    if (MouseWheel || MouseKeys[M3])
        UpdateAABB();

    if (GameStateTicks[GAME_STATE_MENU] != INACTIVE)
        goto END;

    if (MouseKeys[M1]) {
        for (i = 0; i < Curve.Order; i++)
            if (AABBClickTicks[i + AABB_TYPE_MENU_COUNT] != INACTIVE)
                CurrentPicked = i;

        if (!LastMouseKeys[M1] && (CurrentPicked == INACTIVE ||
                AABBClickTicks[CurrentPicked + AABB_TYPE_MENU_COUNT] == INACTIVE)) {
            CurrentPicked = Curve.Order;
            Curve.CtrlPnts[Curve.Order++] = RendererScreenToWorld(SnappedPosition);
            UpdateCurve();
        }

    }

    if (Keys['d'] && !LastKeys['d'] && CurrentPicked != INACTIVE) {
        DeleteControlPoint(CurrentPicked);
        CurrentPicked = INACTIVE;
        goto END;
    }

    if (CurrentPicked != INACTIVE && MouseKeys[M1]) {
        if (AABBClickTicks[CurrentPicked + AABB_TYPE_MENU_COUNT] != INACTIVE) 
            IsEditing = TRUE;
    } else {
        IsEditing = FALSE;
    }
    
    if (IsEditing) {
        Curve.CtrlPnts[CurrentPicked] = RendererScreenToWorld(SnappedPosition);
        UpdateCurve();
    }

END:
    RendererPopTransform();
    RendererPopTransform();
}

void CurvesDraw()
{
    IntType Iters,
        XOffset = (int)((int)Pos.x / (WIN_WIDTH)) * (WIN_WIDTH),
        YOffset = (int)((int)Pos.y / (WIN_HEIGHT)) * (WIN_HEIGHT);
    RealType
        t = MIN((Tick - GameStateTicks[GAME_STATE_CURVES]) / 50.f, 1.f);

    t = MAX(MIN(t, 1.f), 0.f);
    t = Casteljau1D(t, EaseIn);
    /* Draw the grid.                                                         */
    RendererPushTransform(POINT(-WIN_CENTRE.x * S_FACTOR + XOffset, 
                                -WIN_CENTRE.y * S_FACTOR + YOffset), 1.f);
    RendererPushTransform(POINT(-Pos.x, -Pos.y), 1.f);
    RendererPushTransform(WIN_CENTRE, Scale);
    RendererSetFGColour(25 * t, 25 * t, 25 * t);
    for (Iters = GRID_SIZE_MINOR; Iters <= WIN_WIDTH * S_FACTOR; Iters += GRID_SIZE_MAJOR)
        RendererDrawLine(POINT(Iters, 0), POINT(Iters, WIN_HEIGHT * S_FACTOR));

    for (Iters = GRID_SIZE_MINOR; Iters <= WIN_HEIGHT * S_FACTOR; Iters += GRID_SIZE_MAJOR)
        RendererDrawLine(POINT(0, Iters), POINT(WIN_WIDTH * S_FACTOR, Iters));

    RendererSetFGColour(75 * t, 75 * t, 75 * t);
    for (Iters = 0; Iters <= WIN_WIDTH * S_FACTOR; Iters += GRID_SIZE_MAJOR)
        RendererDrawLine(POINT(Iters, 0), POINT(Iters, WIN_HEIGHT * S_FACTOR));

    for (Iters = 0; Iters <= WIN_HEIGHT * S_FACTOR; Iters += GRID_SIZE_MAJOR)
        RendererDrawLine(POINT(0, Iters), POINT(WIN_WIDTH * S_FACTOR, Iters));

    RendererPopTransform();
    RendererPopTransform();
    RendererPopTransform();

    if (!Curve.Order)
        return;

    /* Set the origin to where Pos is.                                        */
    RendererPushTransform(POINT(-Pos.x, -Pos.y), 1.f);
    /* Set the zoom and move everything to the middle of the screen.          */
    RendererPushTransform(WIN_CENTRE, Scale);

    RendererSetFGColour(0, 255 * t, 0);
    for (Iters = 0; Iters < TESSELATION_RES - 1; Iters++)
        RendererDrawLine(EvalCurve[Iters], EvalCurve[Iters + 1]);

    for (Iters = 0; Iters < Curve.Order - 1; Iters++) {
        RendererSetFGColour(255 * t, 0, 0);
        RendererDrawLine(Curve.CtrlPnts[Iters], Curve.CtrlPnts[Iters + 1]);

        if (Iters == CurrentPicked)
            RendererSetFGColour(0, 0, 255 * t);
        else if (AABBHighlightTicks[Iters + AABB_TYPE_MENU_COUNT] != INACTIVE)
            RendererSetFGColour(0, 255 * t, 255 * t);
        else
            RendererSetFGColour(255 * t, 255 * t, 255 * t);
        RendererFillCircle(Curve.CtrlPnts[Iters], 10);
    }

    if (Iters == CurrentPicked)
        RendererSetFGColour(0, 0, 255 * t);
    else if (AABBHighlightTicks[Iters + AABB_TYPE_MENU_COUNT] != INACTIVE)
        RendererSetFGColour(0, 255 * t, 255 * t);
    else
        RendererSetFGColour(255 * t, 255 * t, 255 * t);

    RendererFillCircle(Curve.CtrlPnts[Iters], 10);
    
    RendererPopTransform();
    RendererPopTransform();
}

void UpdateCurve() 
{
    RealType t;
    IntType Iters;

    UpdateAABB();
    for (Iters = 0; Iters < TESSELATION_RES; Iters++) {
        t = Iters / (RealType)(TESSELATION_RES - 1); 
        EvalCurve[Iters] = Casteljau(t, Curve);
    }
}

void UpdateAABB()
{
    IntType Iters;
    PointStruct p, p1, p2;

    for (Iters = 0; Iters < Curve.Order; Iters++) {
        p = Curve.CtrlPnts[Iters];
        p1 = POINT(p.x - 10, p.y - 10);
        p2 = POINT(p.x + 10, p.y + 10);

        AABB_SET(AABB[Iters + AABB_TYPE_MENU_COUNT], p1, p2);
    }
}

void DeleteControlPoint(IntType Index)
{
    for (; Index < Curve.Order; Index++)
        Curve.CtrlPnts[Index] = Curve.CtrlPnts[Index + 1];

    Curve.Order--;
    UpdateAABB();
    UpdateCurve();
}
