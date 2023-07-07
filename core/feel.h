#ifndef FEEL_H
#define FEEL_H
#include "proton.h"

#define CURVE_MAX_ORDER (1024)

typedef enum CurveTypeEnum {
    CURVE_TYPE_BEZIER,
    CURVE_TYPE_BSPLINE
} CurveTypeEnum;

typedef enum EndConditionsEnum {
    END_CONDITIONS_FLOATING,
    END_CONDITIONS_OPEN,
    END_CONDITIONS_UNIFORM_FLOATING,
    END_CONDITIONS_UNIFORM_OPEN,
} EndConditionsEnum;

typedef struct CurveStruct {
    PointStruct CtrlPnts[CURVE_MAX_ORDER];
    RealType KnotVector[CURVE_MAX_ORDER];
    EndConditionsEnum EndConditions;
    CurveTypeEnum Type;
    IntType NumPoints, Degree;
    BoolType NeedsRefresh, NeedsKnots;
} CurveStruct;

typedef struct Curve1DStruct {
    RealType CtrlPnts[CURVE_MAX_ORDER];
    IntType NumPoints;
} Curve1DStruct;

extern Curve1DStruct EaseIn;
extern Curve1DStruct EaseOut;
extern Curve1DStruct EaseInOut;
extern Curve1DStruct EaseOutIn;

PointStruct CurveEval(RealType t, CurveStruct Curve);
PointStruct CurveEvalFirstDeriv(RealType t, CurveStruct Curve);
PointStruct CurveEvalSecondDeriv(RealType t, CurveStruct Curve);
PointStruct BsplineEval(RealType t, CurveStruct BSpline);
PointStruct BezierEval(RealType t, CurveStruct Bezier);
RealType BezierEval1D(RealType t, Curve1DStruct Curve);

/* The required amount of knots that are needed to evaluate this curve.       */
static inline IntType BSplineNumKnots(CurveStruct Curve) 
{
    return Curve.NumPoints + Curve.Degree + 1;
}

#endif /* FEEL_H */
