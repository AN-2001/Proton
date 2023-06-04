#include "curves.h"
#include "feel.h"
#include "proton.h"
#include "renderer.h"
#include "config.h"
    
static CurveStruct 
    Curves[CURVES_TOTAL];
static PointStruct
    EvalCurves[CURVES_TOTAL][TESSELATION_RES];
static IntType
    NumCurves = 0,
    PickedCurve = INACTIVE,
    PickedControlPoint = INACTIVE;

static inline void AddControlPoint(PointStruct Pos);
static inline void SetPickedPoint(PointStruct Pos);
static inline void DeletePickedPoint();
static inline void DrawEvalCurve(IntType ID);
static inline void DrawCtrlPolygon(IntType ID);
static inline void DrawGrid();
static inline void Refresh();
static inline void AddEmptyCurve();
static inline void AddCurveFromFile();

void CurvesUpdate(RealType Delta)
{


}

void CurvesDraw()
{
    IntType i;

    ProtonSetBG(BACKGROUND_COLOUR);

    /* Draw the evaluated curve for everything but the picked curve.          */
    for (i = 0; i < NumCurves; i++) {
        if (i == PickedCurve)
            continue;
        DrawEvalCurve(i);
    }

    /* Draw the control polygon for everything but the picked curve.          */
    for (i = 0; i < NumCurves; i++) {
        if (i == PickedCurve)
            continue;
        DrawCtrlPolygon(i);
    }

    DrawEvalCurve(PickedCurve);
    DrawCtrlPolygon(PickedCurve);
}

void Refresh()
{

}

void DrawEvalCurve(IntType ID)
{
    ProtonSetFG(CURVE_COLOUR);
}

void DrawCtrlPolygon(IntType ID)
{
    /* Draw the control polygon then the points.                              */
    ProtonSetFG(CONTROL_POLY_COLOUR);

    ProtonSetFG(CONTROL_POINT_COLOUR);
}

void AddControlPoint(PointStruct Pos)
{
    if (PickedCurve == INACTIVE)
        return;

    Curves[PickedCurve].CtrlPnts[Curves[PickedCurve].Order++] = Pos;
    Refresh();
}

void SetPickedPoint(PointStruct Pos)
{
    if (PickedCurve == INACTIVE)
        return;

    Curves[PickedCurve].CtrlPnts[PickedCurve] = Pos;
    Refresh();
}

void DeletePickedPoint()
{
    IntType 
        i = PickedControlPoint;
    if (PickedCurve == INACTIVE)
        return;

    for (; i < Curves[PickedCurve].Order; i++)
        Curves[PickedCurve].CtrlPnts[i] = Curves[PickedCurve].CtrlPnts[i + 1]; 

    PickedControlPoint = MAX(PickedControlPoint - 1, -1);
    Curves[PickedCurve].Order = MAX(Curves[PickedCurve].Order - 1, 0);
    Refresh();
}

void AddEmptyCurve()
{
    PickedCurve = NumCurves;
    NumCurves++;
}
