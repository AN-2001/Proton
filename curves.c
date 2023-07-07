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
    AABBStarts[CURVES_TOTAL],
    NewlyAdded[CURVES_TOTAL];
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
static inline void DoConnectionDiffC0(CurveStruct *C1, CurveStruct *C2);
static inline void DoConnectionC0(CurveStruct *C1, CurveStruct *C2);
static inline void DoConnectionC1(CurveStruct *C1, CurveStruct *C2);
static inline void DoConnectionG1(CurveStruct *C1, CurveStruct *C2);
static inline void DumpCurveToFile(CurveStruct C, FILE *File);
static inline void ConvertBezierToBSpline(CurveStruct *C);

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

    if (PickedCtrlPoint == INACTIVE) {
        Zoom *= exp(MouseWheel * Delta * 1e1);
        Zoom = MAX(MIN(Zoom, 10.f), 1 / 2.f);
    }


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
            SnappedPosition.w = PickedCurve -> CtrlPnts[PickedCtrlPoint].w;

            PickedCurve -> CtrlPnts[PickedCtrlPoint] =
                ProtonScreenToWorld(SnappedPosition);
            
            PickedCurve -> CtrlPnts[PickedCtrlPoint].w *=
                        exp(MouseWheel * Delta * 1e1);

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

    if (IsLoading) {
        DrawGhost();
        return;
    }

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
            p = Curves[i].CtrlPnts[j]; AABB_SET(k,
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
        if (TIMER(AABB_HOVER + AABBStarts[ID] + i) != INACTIVE) {
            ProtonSetFG(WEIGHT_RAD_COLOUR);
            ProtonDrawCircle(Curves[ID].CtrlPnts[i],
                             CTRL_POINTS_RADIUS * Curves[ID].CtrlPnts[i].w);
        }
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

static BoolType ShouldLineBeIgnored(const char *Buff)
{
    for (;*Buff == ' ' || *Buff == '\t'; Buff++);
    if (*Buff == '\n' || !*Buff || *Buff == '#')
        return TRUE;

    return FALSE;
}

static BoolType ReadNextInput(char *Buff, FILE *File)
{
    do {
        if (!fgets(Buff, BUFF_SIZE, File))
            return FALSE;
    } while (ShouldLineBeIgnored(Buff));
    return TRUE;
}

void CurvesAddFromPath(const char *Path)
{
    char
        Buff[BUFF_SIZE];
    FILE 
        *DataFile = fopen(Path, "r");
    IntType Order, i, n, NumKnots, t, j, c, NumPoints,
        Knots = 0;
    RealType x, y, w, k;

    for (i = 0; i < CURVES_TOTAL; i++)
        NewlyAdded[i] = -1;

    t = 0;
    while (ReadNextInput(Buff, DataFile)) {

        if ((i = sscanf(Buff, "%d\n", &Order)) != 1) {
            ProtonLogError("Bad input file.");
            goto PANIC;
        }

        if (!ReadNextInput(Buff, DataFile)) {
            ProtonLogError("Bad input file.");
            goto PANIC;
        }

        if ((i = sscanf(Buff, " \tknots[%d] =%n", &Knots, &n) == 1)) {
            ProtonLogInfo("Detected B-Spline with %d knots and %d order", Knots, Order);
            CurvesAddEmptyBspline();
            PickedCurve -> Degree = Order - 1;
            PickedCurve -> EndConditions = END_CONDITIONS_FLOATING;
            NumPoints = Knots - Order;
            memmove(Buff, Buff + n, BUFF_SIZE - n);
        } else {
            CurvesAddEmptyBezier();
            PickedCurve -> Degree = Order - 1;
            NumPoints = Order;
            ProtonLogInfo("Detected bezier with %d control points", Order);
        }
        
        NewlyAdded[t++] = PickedCurveIndx;
        i = 0;
        NumKnots = Knots;
        while (Knots) {
            if (sscanf(Buff, "%f%n", &k, &n) != 1) {
                if (!ReadNextInput(Buff, DataFile)) {
                    ProtonLogError("Bad input file.");
                    goto PANIC;
                }
            } else  {
                PickedCurve -> KnotVector[i++] = k;
                memmove(Buff, Buff + n, BUFF_SIZE - n);
                Knots--;
            }
        }

        for (i = 0; i < NumKnots; i++)
            PickedCurve -> KnotVector[i] = PickedCurve -> KnotVector[i] /
                                           PickedCurve -> KnotVector[NumKnots - 1];

        if (NumKnots && !ReadNextInput(Buff, DataFile)) {
            ProtonLogError("Bad input file.");
            goto PANIC;
        }

        do {
            if ((i = sscanf(Buff, "%f %f %f\n", &x, &y, &w)) == 2) { 
                AddControlPoint(POINT(x, -y));
            } else if (i == 3) {
                AddControlPoint(POINT_W((x / w), -(y / w), w));
            } else {
                ProtonLogError("Bad input file.");
                goto PANIC;
            }
            NumPoints--;
        } while (NumPoints && ReadNextInput(Buff, DataFile));
    }

PANIC:
    EditAngle = 0;
    EditZoom = 10.f;

    t = 0;
    for (j = 0; NewlyAdded[j] != -1; j++) {
        c = NewlyAdded[j];
        for (i = 0; i < Curves[c].NumPoints; i++) {
            Mid.x += Curves[c].CtrlPnts[i].x;
            Mid.y += Curves[c].CtrlPnts[i].y;
            t++;
        }
        PickedCurveIndx = c;
        PickedCurve = &Curves[c];
        Refresh();
    }

    Mid.x = Mid.x / t;
    Mid.y = Mid.y / t;

    PickedCtrlPoint = INACTIVE;
    PickedCurveIndx = INACTIVE;
    PickedCurve = NULL;
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
    IntType i, j, k;

    if (Keys['r']) {
        EditAngle += MouseWheel * Delta * 1e1;
        EditAngle = fmodf(EditAngle, M_PI * 2);
    } else {
        EditZoom *= exp(MouseWheel * Delta * 1e1);
    }

    if (Buttons[M1] && !LastButtons[M1]) {
        ProtonPushTransform();
        ProtonTranslate(POINT(-Mid.x, -Mid.y));
        ProtonScale(EditZoom);
        ProtonRotate(EditAngle);
        ProtonTranslate(SnappedPosition);
        for (j = 0; NewlyAdded[j] != -1; j++) {
            k = NewlyAdded[j];
            for (i = 0; i < Curves[k].NumPoints; i++) {
                Curves[k].CtrlPnts[i] = ProtonWorldToScreen(Curves[k].CtrlPnts[i]);
            }
        }
        ProtonPopTransform();
        
        ProtonPushTransform();
        ProtonTranslate(POINT(-Pan.x, -Pan.y));
        ProtonScale(Zoom);
        ProtonTranslate(WIN_CENTRE);
        for (j = 0; NewlyAdded[j] != -1; j++) {
            k = NewlyAdded[j];
            for (i = 0; i < Curves[k].NumPoints; i++) {
                Curves[k].CtrlPnts[i] = ProtonScreenToWorld(Curves[k].CtrlPnts[i]);
            }
        }

        RefreshAABB();
        for (j = 0; NewlyAdded[j] != -1; j++) {
            PickedCurveIndx = NewlyAdded[j];
            PickedCurve = &Curves[PickedCurveIndx];
            Refresh();
        }
        ProtonPopTransform();

        IsLoading = FALSE;
        PickedCurve = NULL;
        PickedCtrlPoint = INACTIVE;
        PickedCurveIndx = INACTIVE;
    }

}

void DrawGhost()
{
    IntType i, k;

    ProtonPushTransform();
    ProtonTranslate(POINT(-Mid.x, -Mid.y));
    ProtonScale(EditZoom);
    ProtonRotate(EditAngle);
    ProtonTranslate(SnappedPosition);
    for (i = 0; NewlyAdded[i] != -1; i++)  {
        k = NewlyAdded[i];
        DrawCurveEval(k);
    }
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
    CurveStruct
        *Curve1 = &Curves[ID1],
        *Curve2 = &Curves[ID2];

    if (ID1 == ID2)
        return;

    /* Convert to b-spline if bezier.                                         */
    if (Curve1 -> Type == CURVE_TYPE_BEZIER)
        ConvertBezierToBSpline(Curve1);
    if (Curve2 -> Type == CURVE_TYPE_BEZIER)
        ConvertBezierToBSpline(Curve2);


    switch (ConnectionType) {
        case CONNECTION_C1:
            DoConnectionC1(Curve1, Curve2);
            break;
        case CONNECTION_G1:
            DoConnectionG1(Curve1, Curve2);
            break;
        default:
            break;
    }
    if (Curve1 -> Degree == Curve2 -> Degree)
        DoConnectionC0(Curve1, Curve2);
    else 
        DoConnectionDiffC0(Curve1, Curve2);
    
    Curve1 -> NumPoints = 0;
    Curve2 -> NumPoints = 0;

    PickedCtrlPoint = INACTIVE;
    Refresh();
    RefreshAABB();
    IsConnecting = FALSE;
}

void DoConnectionC0(CurveStruct *C1, CurveStruct *C2)
{
    RealType OffsetX, OffsetY;
    IntType i, k,
        LenKnots1 = BSplineNumKnots(*C1),
        LenKnots2 = BSplineNumKnots(*C2),
        Middle = C1 -> Degree,
        Len1 = C1 -> NumPoints,
        Len2 = C2 -> NumPoints;

    OffsetX = C1 -> CtrlPnts[Len1 - 1].x - C2 -> CtrlPnts[0].x;
    OffsetY = C1 -> CtrlPnts[Len1 - 1].y - C2 -> CtrlPnts[0].y;

    CurvesAddEmptyBspline();
    PickedCurve -> Degree = C1 -> Degree;

    for (i = 0; i < Len1; i++) {
        AddControlPoint(C1 -> CtrlPnts[i]);
    }

    for (i = 1; i < Len2; i++) {
        AddControlPoint(POINT_W(C2 -> CtrlPnts[i].x + OffsetX,
                                C2 -> CtrlPnts[i].y + OffsetY,
                                C2 -> CtrlPnts[i].w));
    }

    k = 0;
    for (i = 0; i < LenKnots1 - Middle - 1; i++)
        PickedCurve -> KnotVector[k++] = C1 -> KnotVector[i] / 2.f;

    for (i = 0; i < Middle; i++)
        PickedCurve -> KnotVector[k++] = C1 -> KnotVector[LenKnots1 - Middle - 1] / 2.f;

    for (i = Middle + 1; i < LenKnots2; i++)
        PickedCurve -> KnotVector[k++] = (C1 -> KnotVector[LenKnots1 - Middle - 1] + (C2 -> KnotVector[i] - C2 -> KnotVector[Middle])) / 2.f;

    for (i = 0; i < k; i++) {
        PickedCurve -> KnotVector[i] = PickedCurve -> KnotVector[i] /
                                       PickedCurve -> KnotVector[k - 1];
    }


    PickedCurve -> EndConditions = END_CONDITIONS_FLOATING;
}

void DoConnectionDiffC0(CurveStruct *C1, CurveStruct *C2)
{
    RealType OffsetX, OffsetY;
    IntType i,
        LenKnots1 = BSplineNumKnots(*C1),
        LenKnots2 = BSplineNumKnots(*C2),
        Len1 = C1 -> NumPoints,
        Len2 = C2 -> NumPoints;

    OffsetX = C1 -> CtrlPnts[Len1 - 1].x - C2 -> CtrlPnts[0].x;
    OffsetY = C1 -> CtrlPnts[Len1 - 1].y - C2 -> CtrlPnts[0].y;

    CurvesAddEmptyBspline();
    PickedCurve -> Degree = C1 -> Degree;
    for (i = 0; i < Len1; i++)
        AddControlPoint(C1 -> CtrlPnts[i]);
    for (i = 0; i < LenKnots1; i++)
        PickedCurve -> KnotVector[i] = C1 -> KnotVector[i];

    for (i = LenKnots1 - C1 -> Degree - 1; i < LenKnots1; i++)
        PickedCurve -> KnotVector[i] = PickedCurve -> KnotVector[LenKnots1 - 1];

    PickedCurve -> EndConditions = END_CONDITIONS_FLOATING;
    Refresh();

    CurvesAddEmptyBspline();
    PickedCurve -> Degree = C2 -> Degree;
    for (i = 0; i < Len2; i++)
        AddControlPoint(POINT_W(C2 -> CtrlPnts[i].x + OffsetX,
                                C2 -> CtrlPnts[i].y + OffsetY,
                                C2 -> CtrlPnts[i].w));
    for (i = 0; i < LenKnots2; i++)
        PickedCurve -> KnotVector[i] = C2 -> KnotVector[i];
    for (i = 0; i <= C2 -> Degree; i++)
        PickedCurve -> KnotVector[i] = C2 -> KnotVector[0];

    PickedCurve -> EndConditions = END_CONDITIONS_FLOATING;
    Refresh();
}

void DoConnectionC1(CurveStruct *C1, CurveStruct *C2)
{
    RealType OffsetX, OffsetY;
    IntType i,
        Len1 = C1 -> NumPoints;
    RealType
        Coeff = C1 -> Degree / (RealType) C2 -> Degree;

    OffsetX = C1 -> CtrlPnts[Len1 - 1].x - C1 -> CtrlPnts[Len1 - 2].x;
    OffsetY = C1 -> CtrlPnts[Len1 - 1].y - C1 -> CtrlPnts[Len1 - 2].y;

    C2 -> CtrlPnts[1].x = C2 -> CtrlPnts[0].x + OffsetX * Coeff;
    C2 -> CtrlPnts[1].y = C2 -> CtrlPnts[0].y + OffsetY * Coeff;
}

void DoConnectionG1(CurveStruct *C1, CurveStruct *C2)
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

void ConvertBezierToBSpline(CurveStruct *C)
{
    IntType i, NumKnots,
        Len = C -> NumPoints;

    C -> Type = CURVE_TYPE_BSPLINE;
    C -> Degree = Len - 1;
    NumKnots = BSplineNumKnots(*C);
    
    for (i = 0; i < Len; i++)
        C -> KnotVector[i] = 0.f;

    for (; i < NumKnots; i++)
        C -> KnotVector[i] = 1.f;

}

void CurvesDumpScene(const char *Path)
{
    IntType i;
    FILE
        *File = fopen(Path, "w");

    fprintf(File, "# Created by Abed Na'aran\n");
    for (i = 0; i < CURVES_TOTAL; i++) {
        if (Curves[i].NumPoints)
            DumpCurveToFile(Curves[i], File);
    }

    fclose(File);
}

void DumpCurveToFile(CurveStruct C, FILE *File)
{
    IntType i, k, 
        Order = C.Degree + 1,
        NumKnots = BSplineNumKnots(C);

    if (C.Type == CURVE_TYPE_BEZIER) {
        fprintf(File, "# Dumping bezier curve\n");
        Order = C.NumPoints;
    } else
        fprintf(File, "# Dumping b-spline curve\n");

    fprintf(File, "%d\n", Order);
    
    if (C.Type == CURVE_TYPE_BSPLINE) {
        fprintf(File, "knots[%d] = ", NumKnots);
        k = NumKnots / 4;
        for (i = 0; i < k; i += 4)
            fprintf(File,
                    "%f %f %f %f \n",
                    C.KnotVector[i],
                    C.KnotVector[i + 1],
                    C.KnotVector[i + 2],
                    C.KnotVector[i + 3]);

        for (;i < NumKnots; i++)
            fprintf(File, " %f ", C.KnotVector[i]);
        fprintf(File, "\n");
    }
    
    for (i = 0; i < C.NumPoints; i++) 
        fprintf(File,
                "%f %f %f \n",
                C.CtrlPnts[i].x * C.CtrlPnts[i].w / 100.f,
                -C.CtrlPnts[i].y * C.CtrlPnts[i].w / 100.f,
                C.CtrlPnts[i].w);

}
