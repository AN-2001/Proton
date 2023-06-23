#include "curves.h"
#include "feel.h"
#include "menu.h"
#include "proton.h"
#include "renderer.h"
#include "palette.h"
#include <math.h>
#include <stdio.h>

#define CTRL_POINTS_RADIUS (5)
#define GRID_SIZE_MAJOR (50)
#define GRID_SIZE_MINOR (GRID_SIZE_MAJOR / 2)
#define S_FACTOR (4.f)
#define INACTIVE_INPUT (-2)
#define FADE_IN_T (50.f)
#define CTRL_POINT_FADE_T (20.f)

typedef struct FrenetFrameStruct {
    PointStruct P, T, N;
    RealType V, Kappa;
} FrenetFrameStruct;

static struct InputReportStruct {
    IntType Curve, CtrlPoint;
} ClickReport = {INACTIVE_INPUT, INACTIVE_INPUT};
static CurveStruct 
    Curves[CURVES_TOTAL];
static PointStruct SnappedPosition, Mid,
    CurveEvals[CURVES_TOTAL][TESSELATION_RES],
    Pan = WIN_CENTRE;
static IntType XOffset, YOffset,
    NumCurves = 0,
    ClosestPoint = INACTIVE,
    PickedCurveIndx = INACTIVE,
    PickedCtrlPoint = INACTIVE,
    AABBStarts[CURVES_TOTAL];
static RealType
    Zoom = 1.f,
    EditZoom = 10.f,
    EditAngle = 0.f;
static BoolType
    NeedsAABBRefresh = FALSE,
    ControlPolygonVisible = TRUE,
    TangentVisible = FALSE,
    NormalVisible = FALSE,
    OsculatingCircleVisible = FALSE,
    IsConnecting = FALSE,
    IsLoading = FALSE;
static ConnectionEnum ConnectionType;
static FrenetFrameStruct FrenetFrame;
    
CurveStruct 
    *PickedCurve = NULL;

static inline void AddControlPoint(PointStruct Pos);
static inline void DeletePickedPoint();
static inline void DrawCurveEval(IntType ID);
static inline void DrawCtrlPolygon(IntType ID);
static inline void DrawCtrlPoints(IntType ID);
static inline void DrawTangent();
static inline void DrawNormal();
static inline void DrawOsculatingCircle();
static inline void UpdateGhost(RealType Delta);
static inline void DrawGhost();
static inline void DrawGrid(RealType t);
static inline void RefreshAABB();
static inline void RefreshKnots();
static inline void KnotsSetOpenEnd();
static inline void Refresh();
static inline void AddCurveFromFile();
static inline void MakeInputReport();
static inline void AddEmptyCurve();
static inline void FindClosestPoint();
static inline void CalcualteFrenetFrame();
static inline void DoConnection(IntType ID1, IntType ID2);
static inline void DoConnectionBezierDiffC0(CurveStruct *C1, CurveStruct *C2);
static inline void DoConnectionBezierC0(CurveStruct *C1, CurveStruct *C2);
static inline void DoConnectionBezierC1(CurveStruct *C1, CurveStruct *C2);
static inline void DoConnectionBezierG1(CurveStruct *C1, CurveStruct *C2);
static inline void DoConnectionBSplineC0(CurveStruct *C1, CurveStruct *C2);
static inline void DoConnectionBSplineC1(CurveStruct *C1, CurveStruct *C2);
static inline void DoConnectionBSplineG1(CurveStruct *C1, CurveStruct *C2);

void CurvesUpdate(RealType Delta)
{
    IntType
        SnapAmount = GRID_SIZE_MINOR;

    SnappedPosition = Mouse;

    if (TIMER(GS_KNOT_EDITOR) != INACTIVE)
        goto END;
    if (TIMER(GS_INPUT_PROMPT) != INACTIVE)
        goto END;
    if (TIMER(GS_FILE_PICKER) != INACTIVE)
        goto END;
    if (TIMER(GS_MENU) != INACTIVE)
        goto END;
    if (TIMER(GS_CURVES) < FADE_IN_T)
        goto END;

    if (Buttons[M2] && !LastButtons[M2] &&
            (TIMER(GS_MENU) == INACTIVE ||
             TIMER(AABB_HOVER + AABB_MENU_ITEM_0) == INACTIVE)) {
        TIMER_RESET(GS_MENU);
        TIMER_STOP(GS_FILE_PICKER);
    }

    XOffset = (IntType)((IntType)Pan.x / (WIN_WIDTH)) * (WIN_WIDTH);
    YOffset = (IntType)((IntType)Pan.y / (WIN_HEIGHT)) * (WIN_HEIGHT);
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
    MakeInputReport();
    if (Buttons[M3]) {
        Pan.x = Pan.x - (Mouse.x - LastMouse.x) / Zoom;
        Pan.y = Pan.y - (Mouse.y - LastMouse.y) / Zoom;
    }
    if (IsLoading) {
        UpdateGhost(Delta);
        return;
    }

    Zoom *= exp(MouseWheel * Delta * 1e1);
    Zoom = MAX(MIN(Zoom, 10.f), 1 / 2.f);


    if (IsConnecting && ClickReport.CtrlPoint != INACTIVE_INPUT)
        DoConnection(PickedCurveIndx, ClickReport.Curve);

    /* Pan to the origin, scale everything there then translate               */
    /* the origin to the middle of the screen.                                */
    ProtonPushTransform();
    ProtonTranslate(POINT(-Pan.x, -Pan.y));
    ProtonScale(Zoom);
    ProtonTranslate(WIN_CENTRE);

    CalcualteFrenetFrame();
    if (Buttons[M3] || MouseWheel || NeedsAABBRefresh)
        NeedsAABBRefresh = TRUE;

    if (!IsConnecting) {
        if (PickedCtrlPoint != INACTIVE) {
            Curves[PickedCurveIndx].CtrlPnts[PickedCtrlPoint] =
                ProtonScreenToWorld(SnappedPosition);
            PickedCurve -> NeedsRefresh = TRUE;
            if (Keys['d'] && !LastKeys['d'])
                DeletePickedPoint();
            if (!Buttons[M1])
                PickedCtrlPoint = INACTIVE;
        } else {
            if (ClickReport.CtrlPoint != INACTIVE_INPUT) {
                PickedCtrlPoint = ClickReport.CtrlPoint;
                PickedCurveIndx = ClickReport.Curve;
                PickedCurve = &Curves[PickedCurveIndx];
            } else if (Buttons[M1] && !LastButtons[M1]) {
                AddControlPoint(ProtonScreenToWorld(SnappedPosition));
            }
        }
    }


    if (NeedsAABBRefresh)
        RefreshAABB();

    ProtonPopTransform();
END:
    if (PickedCurve && PickedCurve -> NeedsKnots)
        RefreshKnots();

    if (PickedCurve && PickedCurve -> NeedsRefresh)
        Refresh();
}

void CurvesDraw()
{
    IntType i;
    RealType t;

    t = ANIM_PARAM(TIMER(GS_CURVES), FADE_IN_T);
    t = BezierEval1D(t, EaseOut);

    ProtonSetBG(ColourMult(BACKGROUND_COLOUR, t));
    DrawGrid(t);

    if (IsLoading)
        DrawGhost();

    /* Trnslate Pan to the original, scale everything there then translate    */
    /* the origin to the middle of the screen.                                */
    ProtonPushTransform();
    ProtonTranslate(POINT(-Pan.x, -Pan.y));
    ProtonScale(Zoom);
    ProtonTranslate(WIN_CENTRE);

    for (i = 0; ControlPolygonVisible && i < NumCurves; i++) {
        if (i == PickedCurveIndx && IsLoading)
            continue;
        DrawCtrlPolygon(i);
    }
    for (i = 0; i < NumCurves; i++) {
        if (i == PickedCurveIndx && IsLoading)
            continue;
        DrawCurveEval(i);
    }
    for (i = 0; ControlPolygonVisible && i < NumCurves; i++) {
        if (i == PickedCurveIndx && IsLoading)
            continue;
        DrawCtrlPoints(i);
    }

    if (PickedCtrlPoint != INACTIVE) {
        ProtonSetFG(PICKED_CONTROL_POINT_COLOUR); 
        ProtonFillCircle(PickedCurve -> CtrlPnts[PickedCtrlPoint],
                         CTRL_POINTS_RADIUS);
    }

    if (OsculatingCircleVisible)
        DrawOsculatingCircle();
    if (TangentVisible)
        DrawTangent();
    if (NormalVisible)
        DrawNormal();

    ProtonPopTransform();
}

void Refresh()
{
    RealType t;
    IntType i;

    if (!PickedCurve || !PickedCurve -> NumPoints)
        return;

    for (i = 0; i < TESSELATION_RES; i++) {
        t = i / (RealType)(TESSELATION_RES - 1);
        CurveEvals[PickedCurveIndx][i] = CurveEval(t, *PickedCurve);
    }

    PickedCurve -> NeedsRefresh = FALSE;
    NeedsAABBRefresh = TRUE;
}

void RefreshKnots()
{
    IntType i, k,
        Degree = PickedCurve -> Degree,
        NumKnots = BSplineNumKnots(*PickedCurve);

    if (PickedCurve -> EndConditions == END_CONDITIONS_OPEN) {
        KnotsSetOpenEnd();
        return;
    }

    if (PickedCurve -> EndConditions != END_CONDITIONS_UNIFORM_FLOATING &&
            PickedCurve -> EndConditions != END_CONDITIONS_UNIFORM_OPEN)
        return;

    if (PickedCurve -> EndConditions == END_CONDITIONS_UNIFORM_FLOATING) {
        for (i = 0; i < NumKnots; i++)
            PickedCurve -> KnotVector[i] = i / (RealType)(NumKnots - 1);
    } else {
        for (i = 0; i <= Degree; i++)
            PickedCurve -> KnotVector[i] = 0;

        for (;i < NumKnots - Degree - 1;
             i++) {
            k = i - Degree;
            PickedCurve -> KnotVector[i] = k / 
                (RealType)(NumKnots - Degree * 2 - 1);
        }

        for (;i < NumKnots;
             i++)
            PickedCurve -> KnotVector[i] = 1.f;

    }

    PickedCurve -> NeedsKnots = FALSE;
}

void KnotsSetOpenEnd()
{
    IntType i,
        NumKnots = BSplineNumKnots(*PickedCurve);

    if (NumKnots != BSplineNumKnots(*PickedCurve))
        return;

    for (i = 1; i <= PickedCurve -> Degree; i++)
        PickedCurve -> KnotVector[i] = PickedCurve -> KnotVector[i - 1];

    for (i = NumKnots - 2;
         i >= NumKnots - PickedCurve -> Degree - 1;
         i--)
        PickedCurve -> KnotVector[i] = PickedCurve -> KnotVector[i + 1];
}

void RefreshAABB()
{
    IntType i, j, k;
    PointStruct p;

    k = AABB_MENU_TOTAL;
    for (i = 0; i < NumCurves; i++) {
        AABBStarts[i] = k;
        for (j = 0; j < Curves[i].NumPoints; j++) {
            p = Curves[i].CtrlPnts[j];
            AABB_SET(k,
                    POINT(p.x - CTRL_POINTS_RADIUS * 2, p.y - CTRL_POINTS_RADIUS * 2),
                    POINT(p.x + CTRL_POINTS_RADIUS * 2, p.y + CTRL_POINTS_RADIUS * 2));
            k++;
        }
    }

    NeedsAABBRefresh = FALSE;
}

void DrawCurveEval(IntType ID)
{
    IntType i; 

    if (!Curves[ID].NumPoints)
        return;

    ProtonSetFG(CURVE_COLOUR);
    for (i = 0; i < TESSELATION_RES - 1; i++) {
        ProtonDrawLine(CurveEvals[ID][i],
                       CurveEvals[ID][i + 1]);
    }


}

void DrawCtrlPolygon(IntType ID)
{
    IntType i;
    ColourStruct c;

    if (!Curves[ID].NumPoints)
        return;
    c = CONTROL_POLY_COLOUR;
    if (ID != PickedCurveIndx) {
        c.a = 128;
    }

    ProtonSetFG(c);
    for (i = 0; i < Curves[ID].NumPoints - 1; i++)
        ProtonDrawLine(Curves[ID].CtrlPnts[i],
                       Curves[ID].CtrlPnts[i + 1]);

}

void DrawCtrlPoints(IntType ID)
{
    IntType i;
    RealType t1, t2, t;
    ColourStruct c1, c2, c;

    if (!Curves[ID].NumPoints)
        return;
    c1 = CONTROL_POINT_COLOUR;
    c2 = PICKED_CONTROL_POINT_COLOUR;

    if (ID != PickedCurveIndx) {
        c1.a = 128;
        c2.a = 128;
    }

    for (i = 0; i < Curves[ID].NumPoints; i++) {
        t1 = ANIM_PARAM(TIMER(AABB_HOVER + AABBStarts[ID] + i), CTRL_POINT_FADE_T);
        t2 = ANIM_PARAM(TIMER(AABB_NHOVER + AABBStarts[ID] + i), CTRL_POINT_FADE_T);

        t1 = BezierEval1D(t1, EaseIn);
        t2 = BezierEval1D(t2, EaseOut);

        t = t1 + (1 - t2) * (TIMER(AABB_NHOVER + AABBStarts[ID] + i) != INACTIVE);
        c = ColourAdd(ColourMult(c1, 1 - t),
                      ColourMult(c2, t));

        ProtonSetFG(c);
        ProtonFillCircle(Curves[ID].CtrlPnts[i], CTRL_POINTS_RADIUS);
    }
}

void AddControlPoint(PointStruct Pos)
{
    if (PickedCurveIndx == INACTIVE)
        return;

    PickedCurve -> CtrlPnts[PickedCurve -> NumPoints] = Pos;
    PickedCtrlPoint = PickedCurve -> NumPoints;
    PickedCurve -> NumPoints++;
    PickedCurve -> NeedsRefresh = TRUE;
    PickedCurve -> NeedsKnots = TRUE;
}

void DeletePickedPoint()
{
    IntType 
        i = PickedCtrlPoint;

    if (!PickedCurve)
        return;
    for (; i < PickedCurve -> NumPoints; i++)
        PickedCurve -> CtrlPnts[i] = PickedCurve -> CtrlPnts[i + 1]; 

    PickedCtrlPoint = INACTIVE;
    PickedCurve -> NumPoints = MAX(PickedCurve -> NumPoints - 1, 0);
    PickedCurve -> NeedsKnots = TRUE;
    PickedCurve -> NeedsRefresh = TRUE;
}

void AddEmptyCurve()
{
    for (PickedCurveIndx = 0; PickedCurveIndx < NumCurves; PickedCurveIndx++) {
        PickedCurve = &Curves[PickedCurveIndx];
        if (!PickedCurve -> NumPoints) {
            return;
        }
    }

    PickedCurve = &Curves[PickedCurveIndx];
    NumCurves++;
}

void CurvesAddEmptyBezier()
{
    AddEmptyCurve();
    PickedCurve -> NeedsRefresh = FALSE;
    PickedCurve -> Type = CURVE_TYPE_BEZIER;
}

void CurvesAddEmptyBspline()
{
    IntType i;

    AddEmptyCurve();
    PickedCurve -> NeedsRefresh = FALSE;
    PickedCurve -> EndConditions = END_CONDITIONS_UNIFORM_FLOATING;
    PickedCurve -> Degree = 1;
    for (i = 0; i < CURVE_MAX_ORDER; i++)
        PickedCurve -> KnotVector[i] = 1.f;
    PickedCurve -> Type = CURVE_TYPE_BSPLINE;
}

void CurvesAddFromPath(const char *Path)
{
    char Buff[BUFF_SIZE];
    FILE 
        *DataFile = fopen(Path, "r");
    IntType NumPoints, i;
    RealType x, y, w;

    while (fgets(Buff, BUFF_SIZE, DataFile)) {
        if (Buff[0] == '#' || !Buff[0])
            continue;

        if ((i = sscanf(Buff, "%d\n", &NumPoints)) != 1) {
            ProtonLogError("Bad input file.");
            goto PANIC;
        }

        CurvesAddEmptyBezier();
        while (NumPoints && fgets(Buff, BUFF_SIZE, DataFile)) {
            if (Buff[0] == '#' || !Buff[0])
                continue;

            if ((i = sscanf(Buff, "%f %f %f\n", &x, &y, &w)) == 2) { 
                AddControlPoint(POINT(x, y));
            } else if (i == 3) {
                AddControlPoint(POINT(x, y));
            } else {
                ProtonLogError("Bad input file.");
                goto PANIC;
            }

            NumPoints--;
        }
    }

PANIC:
    EditAngle = 0;
    EditZoom = 10.f;
    for (i = 0; i < Curves[PickedCurveIndx].NumPoints; i++) {
        Mid.x += PickedCurve -> CtrlPnts[i].x;
        Mid.y += PickedCurve -> CtrlPnts[i].y;
    }
    Mid.x = -Mid.x / PickedCurve -> NumPoints;
    Mid.y = -Mid.y / PickedCurve -> NumPoints;

    PickedCurve -> NeedsRefresh = TRUE;
    PickedCtrlPoint = INACTIVE;
    IsLoading = TRUE;
    fclose(DataFile);
}

void MakeInputReport()
{
    IntType i, j;

    ClickReport = (struct InputReportStruct){INACTIVE_INPUT, INACTIVE_INPUT};

    /* Loop in reverse so that the latest curve is picked first.              */
    for (i = NumCurves - 1; i >= 0; i--) {
        for (j = 0; j < Curves[i].NumPoints; j++) {
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


void UpdateGhost(RealType Delta)
{
    IntType i;

    if (Keys['r']) {
        EditAngle += MouseWheel * Delta * 1e1;
        EditAngle = fmodf(EditAngle, M_PI * 2);
    } else {
        EditZoom *= exp(MouseWheel * Delta * 1e1);
    }

    for (i = 0; i < Curves[PickedCurveIndx].NumPoints; i++) {
        Mid.x += PickedCurve -> CtrlPnts[i].x;
        Mid.y += PickedCurve -> CtrlPnts[i].y;
    }

    Mid.x = -Mid.x / PickedCurve -> NumPoints;
    Mid.y = -Mid.y / PickedCurve -> NumPoints;

    if (Buttons[M1] && !LastButtons[M1]) {
        ProtonPushTransform();
        ProtonTranslate(Mid);
        ProtonScale(EditZoom);
        ProtonRotate(EditAngle);
        ProtonTranslate(SnappedPosition);
        for (i = 0; i < Curves[PickedCurveIndx].NumPoints; i++)
            PickedCurve -> CtrlPnts[i] =
                ProtonWorldToScreen(PickedCurve -> CtrlPnts[i]);
        ProtonPopTransform();
        
        IsLoading = FALSE;
        ProtonPushTransform();
        ProtonTranslate(POINT(-Pan.x, -Pan.y));
        ProtonScale(Zoom);
        ProtonTranslate(WIN_CENTRE);
        for (i = 0; i < Curves[PickedCurveIndx].NumPoints; i++)
            PickedCurve -> CtrlPnts[i] =
                ProtonScreenToWorld(PickedCurve -> CtrlPnts[i]);
        PickedCurve -> NeedsRefresh = TRUE;
        ProtonPopTransform();
    }

}

void DrawGhost()
{
    IntType i;

    ProtonPushTransform();
    ProtonTranslate(Mid);
    ProtonScale(EditZoom);
    ProtonRotate(EditAngle);
    ProtonTranslate(SnappedPosition);
    DrawCurveEval(PickedCurveIndx);
    ProtonPopTransform();
}

void FindClosestPoint()
{
    IntType i;
    PointStruct P,
        M = ProtonScreenToWorld(Mouse);
    RealType Dist,
        MinDist = INFINITY;

    if (!PickedCurve)
        return;

    for (i = 0; i < TESSELATION_RES; i++) {
        P = CurveEvals[PickedCurveIndx][i];
        Dist = sqrt(pow(M.x - P.x, 2) + pow(M.y - P.y, 2));
        if (Dist < MinDist) {
            ClosestPoint = i;
            MinDist = Dist;
        }
    }

}

void CurvesToggleTangent()
{
    TangentVisible = !TangentVisible;

}

void CurvesToggleNormal()
{
    NormalVisible = !NormalVisible;

}

void CurvesToggleOsculatingCircle()
{
    OsculatingCircleVisible = !OsculatingCircleVisible;

}

void CurvesToggleControlPolygon()
{
    ControlPolygonVisible = !ControlPolygonVisible;
}

void DrawTangent()
{
    PointStruct Start, End;

    Start.x = FrenetFrame.P.x;
    Start.y = FrenetFrame.P.y;
    End.x = FrenetFrame.P.x + FrenetFrame.T.x * 100.f;
    End.y = FrenetFrame.P.y + FrenetFrame.T.y * 100.f;

    ProtonSetFG(COLOUR_WHITE);
    ProtonFillCircle(FrenetFrame.P, CTRL_POINTS_RADIUS);
    ProtonSetFG(TANGENT_COLOUR);
    ProtonDrawLine(FrenetFrame.P, End);
}

void DrawNormal()
{
    PointStruct Start, End;

    if (!TangentVisible) {
        Start.x = FrenetFrame.P.x;
        Start.y = FrenetFrame.P.y;
    } else {
        Start.x = FrenetFrame.P.x + FrenetFrame.T.x * 100;
        Start.y = FrenetFrame.P.y + FrenetFrame.T.y * 100;
    }

    End.x = Start.x + FrenetFrame.N.x * 100;
    End.y = Start.y + FrenetFrame.N.y * 100;

    ProtonSetFG(COLOUR_WHITE);
    ProtonFillCircle(FrenetFrame.P, CTRL_POINTS_RADIUS);
    ProtonSetFG(NORMAL_COLOUR);
    ProtonDrawLine(Start, End);
}

void DrawOsculatingCircle()
{
    PointStruct Centre;
    RealType
        R = FrenetFrame.Kappa < EPS ? 0 : 1.f / FrenetFrame.Kappa;

    Centre.x = FrenetFrame.P.x + FrenetFrame.N.x * R;
    Centre.y = FrenetFrame.P.y + FrenetFrame.N.y * R;

    ProtonSetFG(COLOUR_WHITE);
    ProtonFillCircle(Centre, CTRL_POINTS_RADIUS);
    ProtonSetFG(OSCULATING_CIRCLE_COLOUR);
    ProtonDrawCircle(Centre, R);
}

void CalcualteFrenetFrame()
{
    if (!PickedCurve)
        return;

    FindClosestPoint();
    RealType Kappa, Len, LenCub, CrossZ,
        t = ClosestPoint / (RealType)(TESSELATION_RES - 1);
    PointStruct
        P0 = CurveEval(t, *PickedCurve),
        P1 = CurveEvalFirstDeriv(t, *PickedCurve),
        P2 = CurveEvalSecondDeriv(t, *PickedCurve);

    Len = sqrt(P1.x * P1.x + P1.y * P1.y);
    LenCub = Len * Len * Len;
	CrossZ = P1.x * P2.y - P1.y * P2.x;

    FrenetFrame.P = P0;
    FrenetFrame.T = P1;
    FrenetFrame.N = POINT(-CrossZ * FrenetFrame.T.y,
                           CrossZ * FrenetFrame.T.x);
    FrenetFrame.V = Len;
    FrenetFrame.Kappa = fabs(CrossZ / LenCub);

    Len = sqrt(FrenetFrame.T.x * FrenetFrame.T.x +
               FrenetFrame.T.y * FrenetFrame.T.y);
    if (Len > EPS) {
        FrenetFrame.T.x = FrenetFrame.T.x / Len;
        FrenetFrame.T.y = FrenetFrame.T.y / Len;
    } else {
        FrenetFrame.T.x = 0;
        FrenetFrame.T.y = 0;
    }

    Len = sqrt(FrenetFrame.N.x * FrenetFrame.N.x +
               FrenetFrame.N.y * FrenetFrame.N.y);
    if (Len > EPS) {
        FrenetFrame.N.x = FrenetFrame.N.x / Len;
        FrenetFrame.N.y = FrenetFrame.N.y / Len;
    } else {
        FrenetFrame.N.x = 0;
        FrenetFrame.N.y = 0;
    }
}

void CurvesConnectCurves(ConnectionEnum Type)
{
    ConnectionType = Type;
    IsConnecting = TRUE;
}

void DoConnection(IntType ID1, IntType ID2)
{
    void (*Funcs[3])(CurveStruct *, CurveStruct *);
    CurveStruct
        *Curve1 = &Curves[ID1],
        *Curve2 = &Curves[ID2];

    if (Curve1 -> Type != Curve2 -> Type)
        return;
    if (ID1 == ID2)
        return;
  
    if (Curve1 -> Type == CURVE_TYPE_BEZIER) {
        if (Curve1 -> NumPoints == Curve2 -> NumPoints)
            Funcs[0] = DoConnectionBezierC0;
        else
            Funcs[0] = DoConnectionBezierDiffC0;
        Funcs[1] = DoConnectionBezierC1;
        Funcs[2] = DoConnectionBezierG1;
    } else {
        Funcs[0] = DoConnectionBSplineC0;
        Funcs[1] = DoConnectionBSplineC1;
        Funcs[2] = DoConnectionBSplineG1;
    }

    switch (ConnectionType) {
        case CONNECTION_C1:
            Funcs[1](Curve1, Curve2);
            break;
        case CONNECTION_G1:
            Funcs[2](Curve1, Curve2);
            break;
        default:
            break;
    }
    Funcs[0](Curve1, Curve2);

    Curves[ID1].NumPoints = 0;
    Curves[ID2].NumPoints = 0;
    PickedCtrlPoint = INACTIVE;
    Refresh();
    RefreshAABB();
    IsConnecting = FALSE;
}

void DoConnectionBezierC0(CurveStruct *C1, CurveStruct *C2)
{
    RealType OffsetX, OffsetY;
    IntType i,
        Len1 = C1 -> NumPoints,
        Len2 = C2 -> NumPoints;

    OffsetX = C1 -> CtrlPnts[Len1 - 1].x - C2 -> CtrlPnts[0].x;
    OffsetY = C1 -> CtrlPnts[Len1 - 1].y - C2 -> CtrlPnts[0].y;

    CurvesAddEmptyBspline();
    PickedCurve -> Degree = Len1 - 1;

    for (i = 0; i < Len1; i++) {
        AddControlPoint(C1 -> CtrlPnts[i]);
    }

    for (i = 1; i < Len2; i++) {
        AddControlPoint(POINT(C2 -> CtrlPnts[i].x + OffsetX,
                              C2 -> CtrlPnts[i].y + OffsetY));
    }

    for (i = 0; i < Len1; i++)
        PickedCurve -> KnotVector[i] = 0;

    for (; i < BSplineNumKnots(*PickedCurve) - Len1; i++) 
        PickedCurve -> KnotVector[i] = 0.5f;

    for (; i < BSplineNumKnots(*PickedCurve); i++) 
        PickedCurve -> KnotVector[i] = 1.f;

    PickedCurve -> EndConditions = END_CONDITIONS_FLOATING;
}

void DoConnectionBezierDiffC0(CurveStruct *C1, CurveStruct *C2)
{
    RealType OffsetX, OffsetY;
    IntType i,
        Len1 = C1 -> NumPoints,
        Len2 = C2 -> NumPoints;

    OffsetX = C1 -> CtrlPnts[Len1 - 1].x - C2 -> CtrlPnts[0].x;
    OffsetY = C1 -> CtrlPnts[Len1 - 1].y - C2 -> CtrlPnts[0].y;

    CurvesAddEmptyBspline();
    PickedCurve -> Degree = Len1 - 1;
    for (i = 0; i < Len1; i++) {
        AddControlPoint(C1 -> CtrlPnts[i]);
    }

    for (i = 0; i < Len1; i++)
        PickedCurve -> KnotVector[i] = 0;

    for (; i < BSplineNumKnots(*PickedCurve); i++) 
        PickedCurve -> KnotVector[i] = 1.f;

    PickedCurve -> EndConditions = END_CONDITIONS_FLOATING;
    Refresh();

    CurvesAddEmptyBspline();
    PickedCurve -> Degree = Len2 - 1;
    for (i = 0; i < Len2; i++) {
        AddControlPoint(POINT(C2 -> CtrlPnts[i].x + OffsetX,
                              C2 -> CtrlPnts[i].y + OffsetY));
    }

    for (i = 0; i < Len2; i++)
        PickedCurve -> KnotVector[i] = 0;

    for (; i < BSplineNumKnots(*PickedCurve); i++) 
        PickedCurve -> KnotVector[i] = 1.f;

    PickedCurve -> EndConditions = END_CONDITIONS_FLOATING;
    Refresh();
}

void DoConnectionBezierC1(CurveStruct *C1, CurveStruct *C2)
{
    RealType OffsetX, OffsetY;
    IntType i,
        Len1 = C1 -> NumPoints;

    OffsetX = C1 -> CtrlPnts[Len1 - 1].x - C1 -> CtrlPnts[Len1 - 2].x;
    OffsetY = C1 -> CtrlPnts[Len1 - 1].y - C1 -> CtrlPnts[Len1 - 2].y;

    C2 -> CtrlPnts[1].x = C2 -> CtrlPnts[0].x + OffsetX;
    C2 -> CtrlPnts[1].y = C2 -> CtrlPnts[0].y + OffsetY;
}

void DoConnectionBezierG1(CurveStruct *C1, CurveStruct *C2)
{
    RealType OffsetX, OffsetY, OffsetXX, OffsetYY;
    RealType L1, L2;
    IntType i,
        Len1 = C1 -> NumPoints,
        Len2 = C2 -> NumPoints;

    OffsetX = C1 -> CtrlPnts[Len1 - 1].x - C1 -> CtrlPnts[Len1 - 2].x;
    OffsetY = C1 -> CtrlPnts[Len1 - 1].y - C1 -> CtrlPnts[Len1 - 2].y;

    L1 = sqrt(OffsetX * OffsetX + OffsetY * OffsetY);
    OffsetX = OffsetX / L1;
    OffsetY = OffsetY / L1;

    OffsetXX = C2 -> CtrlPnts[1].x - C2 -> CtrlPnts[0].x;
    OffsetYY = C2 -> CtrlPnts[1].y - C2 -> CtrlPnts[0].y;
    L2 = sqrt(OffsetXX * OffsetXX + OffsetYY * OffsetYY);

    C2 -> CtrlPnts[1].x = C2 -> CtrlPnts[0].x + OffsetX * L2;
    C2 -> CtrlPnts[1].y = C2 -> CtrlPnts[0].y + OffsetY * L2;
}

void DoConnectionBSplineC0(CurveStruct *C1, CurveStruct *C2)
{
    RealType OffsetX, OffsetY;
    IntType i,
        Len1 = C1 -> NumPoints,
        Len2 = C2 -> NumPoints;

    OffsetX = C1 -> CtrlPnts[Len1 - 1].x - C2 -> CtrlPnts[0].x;
    OffsetY = C1 -> CtrlPnts[Len1 - 1].y - C2 -> CtrlPnts[0].y;

    CurvesAddEmptyBspline();
    PickedCurve -> Degree = Len1 - 1;

    for (i = 0; i < Len1; i++) {
        AddControlPoint(C1 -> CtrlPnts[i]);
    }

    for (i = 1; i < Len2; i++) {
        AddControlPoint(POINT(C2 -> CtrlPnts[i].x + OffsetX,
                              C2 -> CtrlPnts[i].y + OffsetY));
    }

    PickedCurve -> EndConditions = END_CONDITIONS_FLOATING;
}

void DoConnectionBSplineC1(CurveStruct *C1, CurveStruct *C2)
{
}

void DoConnectionBSplineG1(CurveStruct *C1, CurveStruct *C2)
{
}
