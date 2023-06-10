#include "curves.h"
#include "feel.h"
#include "proton.h"
#include "renderer.h"
#include "palette.h"

#define CTRL_POINTS_RADIUS (10)
#define GRID_SIZE_MAJOR (50)
#define GRID_SIZE_MINOR (GRID_SIZE_MAJOR / 2)
#define S_FACTOR (4.f)
  
#define INACTIVE_INPUT (-2)
#define FADE_IN_T (50.f)
#define CTRL_POINT_FADE_T (20.f)
static struct InputReportStruct {
    IntType Curve, CtrlPoint;
} ClickReport = {INACTIVE_INPUT, INACTIVE_INPUT};
static CurveStruct 
    Curves[CURVES_TOTAL];
static PointStruct
    EvalCurves[CURVES_TOTAL][TESSELATION_RES],
    Pan = WIN_CENTRE;
static IntType XOffset, YOffset,
    NumCurves = 0,
    PickedCurve = INACTIVE,
    PickedCtrlPoint = INACTIVE,
    AABBStarts[CURVES_TOTAL];
static RealType
    Zoom = 1.f;

static inline void AddControlPoint(PointStruct Pos);
static inline void SetPickedCurve(IntType ID);
static inline void DeletePickedPoint();
static inline void DrawEvalCurve(IntType ID);
static inline void DrawCtrlPolygon(IntType ID);
static inline void DrawCtrlPoints(IntType ID);
static inline void DrawGrid(RealType t);
static inline void RefreshAABB();
static inline void Refresh();
static inline void AddEmptyCurve();
static inline void AddCurveFromFile();
static inline void MakeInputReport();

void CurvesUpdate(RealType Delta)
{
    IntType
        SnapAmount = GRID_SIZE_MINOR;
    PointStruct
        SnappedPosition = Mouse;

    if (TIMER(GS_CURVES) == 0 || (Buttons[M2] && !LastButtons[M2]))
        AddEmptyCurve();

    if (TIMER(GS_CURVES) < FADE_IN_T)
        return;

    XOffset = (IntType)((IntType)Pan.x / (WIN_WIDTH)) * (WIN_WIDTH);
    YOffset = (IntType)((IntType)Pan.y / (WIN_HEIGHT)) * (WIN_HEIGHT);
    MakeInputReport();

    Zoom *= exp(MouseWheel * Delta * 1e1);
    Zoom = MAX(MIN(Zoom, 10.f), 1 / 2.f);
    if (Buttons[M3]) {
        Pan.x = Pan.x - (Mouse.x - LastMouse.x) / Zoom;
        Pan.y = Pan.y - (Mouse.y - LastMouse.y) / Zoom;
    }

    if (Keys['s']) {
        if (Keys['a'])
            SnapAmount = GRID_SIZE_MAJOR;

        ProtonPushTransform();
        ProtonTranslate(POINT(-WIN_CENTRE.x * S_FACTOR + XOffset, 
                              -WIN_CENTRE.y * S_FACTOR + YOffset));
        ProtonTranslate(POINT(-Pan.x, -Pan.y));
        ProtonScale(Zoom);
        ProtonTranslate(WIN_CENTRE);

        SnappedPosition = ProtonScreenToWorld(SnappedPosition);
        SnappedPosition.x = roundf(SnappedPosition.x / SnapAmount) * SnapAmount;
        SnappedPosition.y = roundf(SnappedPosition.y / SnapAmount) * SnapAmount;
        SnappedPosition = ProtonWorldToScreen(SnappedPosition);

        ProtonPopTransform();
    }

    /* Trnslate Pan to the original, scale everything there then translate    */
    /* the origin to the middle of the screen.                                */
    ProtonPushTransform();
    ProtonTranslate(POINT(-Pan.x, -Pan.y));
    ProtonScale(Zoom);
    ProtonTranslate(WIN_CENTRE);

    if (Buttons[M3] || MouseWheel)
        RefreshAABB();

    if (PickedCtrlPoint != INACTIVE) {
        Curves[PickedCurve].CtrlPnts[PickedCtrlPoint] =
            ProtonScreenToWorld(SnappedPosition);
        Refresh();
        if (Keys['d'] && !LastKeys['d'])
            DeletePickedPoint();
        if (!Buttons[M1])
            PickedCtrlPoint = INACTIVE;
    } else {
        if (ClickReport.CtrlPoint != INACTIVE_INPUT) {
            PickedCtrlPoint = ClickReport.CtrlPoint;
            PickedCurve = ClickReport.Curve;
        } else if (Buttons[M1] && !LastButtons[M1]) {
            AddControlPoint(ProtonScreenToWorld(SnappedPosition));
            Refresh();
        }
    }

    ProtonPopTransform();
}

void CurvesDraw()
{
    IntType i;
    RealType t;

    t = ANIM_PARAM(TIMER(GS_CURVES), FADE_IN_T);
    t = Casteljau1D(t, EaseOut);

    ProtonSetBG(ColourMult(BACKGROUND_COLOUR, t));
    DrawGrid(t);

    /* Trnslate Pan to the original, scale everything there then translate    */
    /* the origin to the middle of the screen.                                */
    ProtonPushTransform();
    ProtonTranslate(POINT(-Pan.x, -Pan.y));
    ProtonScale(Zoom);
    ProtonTranslate(WIN_CENTRE);

    for (i = 0; i < NumCurves; i++)
        DrawCtrlPolygon(i);
    for (i = 0; i < NumCurves; i++)
        DrawEvalCurve(i);
    for (i = 0; i < NumCurves; i++)
        DrawCtrlPoints(i);

    if (PickedCtrlPoint != INACTIVE) {
        ProtonSetFG(PICKED_CONTROL_POINT_COLOUR); 
        ProtonFillCircle(Curves[PickedCurve].CtrlPnts[PickedCtrlPoint],
                         CTRL_POINTS_RADIUS);
    }

    ProtonPopTransform();
}

void Refresh()
{
    RealType t;
    IntType i;

    if (PickedCurve == INACTIVE)
        return;

    for (i = 0; i < TESSELATION_RES; i++) {
        t = i / (RealType)(TESSELATION_RES - 1);
        EvalCurves[PickedCurve][i] = Casteljau(t, Curves[PickedCurve]);
    }
    RefreshAABB();
}

void RefreshAABB()
{
    IntType i, j, k;
    PointStruct p;

    k = AABB_MENU_TOTAL;
    for (i = 0; i < NumCurves; i++) {
        AABBStarts[i] = k;
        for (j = 0; j < Curves[i].Order; j++) {
            p = Curves[i].CtrlPnts[j];
            AABB_SET(k,
                    POINT(p.x - CTRL_POINTS_RADIUS, p.y - CTRL_POINTS_RADIUS),
                    POINT(p.x + CTRL_POINTS_RADIUS, p.y + CTRL_POINTS_RADIUS));
            k++;
        }
    }

}

void DrawEvalCurve(IntType ID)
{
    IntType i; 

    ProtonSetFG(CURVE_COLOUR);
    for (i = 0; i < TESSELATION_RES - 1; i++)
        ProtonDrawLine(EvalCurves[ID][i],
                       EvalCurves[ID][i + 1]);
}

void DrawCtrlPolygon(IntType ID)
{
    IntType i;

    ColourStruct c;

    c = CONTROL_POLY_COLOUR;
    if (ID != PickedCurve) {
        c.a = 128;
    }

    ProtonSetFG(c);
    for (i = 0; i < Curves[ID].Order - 1; i++)
        ProtonDrawLine(Curves[ID].CtrlPnts[i],
                       Curves[ID].CtrlPnts[i + 1]);

}

void DrawCtrlPoints(IntType ID)
{
    IntType i;
    RealType t1, t2, t;
    ColourStruct c1, c2, c;

    c1 = CONTROL_POINT_COLOUR;
    c2 = PICKED_CONTROL_POINT_COLOUR;

    if (ID != PickedCurve) {
        c1.a = 128;
        c2.a = 128;
    }

    for (i = 0; i < Curves[ID].Order; i++) {
        t1 = ANIM_PARAM(TIMER(AABB_HOVER + AABBStarts[ID] + i), CTRL_POINT_FADE_T);
        t2 = ANIM_PARAM(TIMER(AABB_NHOVER + AABBStarts[ID] + i), CTRL_POINT_FADE_T);

        t1 = Casteljau1D(t1, EaseIn);
        t2 = Casteljau1D(t2, EaseOut);

        t = t1 + (1 - t2) * (TIMER(AABB_NHOVER + AABBStarts[ID] + i) != INACTIVE);
        c = ColourAdd(ColourMult(c1, 1 - t),
                      ColourMult(c2, t));

        ProtonSetFG(c);
        ProtonFillCircle(Curves[ID].CtrlPnts[i], CTRL_POINTS_RADIUS);
    }
}

void AddControlPoint(PointStruct Pos)
{
    if (PickedCurve == INACTIVE)
        return;

    Curves[PickedCurve].CtrlPnts[Curves[PickedCurve].Order] = Pos;
    PickedCtrlPoint = Curves[PickedCurve].Order;
    Curves[PickedCurve].Order++;
}

void DeletePickedPoint()
{
    IntType 
        i = PickedCtrlPoint;
    if (PickedCurve == INACTIVE)
        return;

    for (; i < Curves[PickedCurve].Order; i++)
        Curves[PickedCurve].CtrlPnts[i] = Curves[PickedCurve].CtrlPnts[i + 1]; 

    PickedCtrlPoint = INACTIVE;
    Curves[PickedCurve].Order = MAX(Curves[PickedCurve].Order - 1, 0);
    Refresh();
}

void SetPickedCurve(IntType ID)
{
    PickedCurve = ID;
    Refresh();
}

void AddEmptyCurve()
{
    for (PickedCurve = 0; PickedCurve < NumCurves; PickedCurve++)
        if (!Curves[PickedCurve].Order)
            return;

    NumCurves++;
}

void MakeInputReport()
{
    IntType i, j;

    ClickReport = (struct InputReportStruct){INACTIVE_INPUT, INACTIVE_INPUT};

    /* Report the very first control point that was clicked.                  */
    for (i = 0; i < NumCurves; i++) {
        for (j = 0; j < Curves[i].Order; j++) {
            if (TIMER(AABB_CLICK + AABBStarts[i] + j) != INACTIVE) {
                ClickReport.Curve = i;
                ClickReport.CtrlPoint = j;
                return;
            }
        }
    }
}

void DrawGrid(RealType t)
{
    IntType i;

    ProtonPushTransform();
    ProtonTranslate(POINT(-WIN_CENTRE.x * S_FACTOR + XOffset, 
                          -WIN_CENTRE.y * S_FACTOR + YOffset));
    ProtonTranslate(POINT(-Pan.x, -Pan.y));
    ProtonScale(Zoom);
    ProtonTranslate(WIN_CENTRE);

    ProtonSetFG(ColourMult(MINOR_GRID_COLOUR, t));
    for (i = GRID_SIZE_MINOR; i <= WIN_WIDTH * S_FACTOR; i += GRID_SIZE_MAJOR)
        ProtonDrawLine(POINT(i, 0), POINT(i, WIN_HEIGHT * S_FACTOR));

    for (i = GRID_SIZE_MINOR; i <= WIN_HEIGHT * S_FACTOR; i += GRID_SIZE_MAJOR)
        ProtonDrawLine(POINT(0, i), POINT(WIN_WIDTH * S_FACTOR, i));

    ProtonSetFG(ColourMult(MAJOR_GRID_COLOUR, t));
    for (i = 0; i <= WIN_WIDTH * S_FACTOR; i += GRID_SIZE_MAJOR)
        ProtonDrawLine(POINT(i, 0), POINT(i, WIN_HEIGHT * S_FACTOR));

    for (i = 0; i <= WIN_HEIGHT * S_FACTOR; i += GRID_SIZE_MAJOR)
        ProtonDrawLine(POINT(0, i), POINT(WIN_WIDTH * S_FACTOR, i));

    ProtonPopTransform();
}
