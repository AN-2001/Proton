#include "feel.h"
#include "proton.h"
#include <string.h>

#define DERIV_H (1e-2)

Curve1DStruct
    EaseIn = {{0.f, 0.f, 0.f, 1.f}, 4},
    EaseOut = {{0.f, 1.f, 1.f, 1.f}, 4},
    EaseInOut = {{0.f, 0.f, 1.f, 1.f}, 4},
    EaseOutIn = {{0.f, 1.f, 0.f, 1.f}, 4};

PointStruct CurveEval(RealType t, CurveStruct Curve)
{
    switch (Curve.Type) {
        case  CURVE_TYPE_BEZIER:
            return BezierEval(t, Curve);
        case CURVE_TYPE_BSPLINE:
            return BsplineEval(t, Curve);
    }
}

PointStruct BsplineEval(RealType t, CurveStruct BSpline)
{
    IntType i, j, p,
        NumKNots = BSplineNumKnots(BSpline);
    PointStruct Old[CURVE_MAX_ORDER], New[CURVE_MAX_ORDER];
    RealType k;

    if (!BSpline.NumPoints)
        return POINT_ZERO;

    t = MIN(MAX(t, BSpline.KnotVector[BSpline.Degree]),
            BSpline.KnotVector[NumKNots - 1 - BSpline.Degree] - EPS);

    for (j = 0;
        j < CURVE_MAX_ORDER - 1 && !(t >= BSpline.KnotVector[j] && t < BSpline.KnotVector[j + 1]);
        j++);

    for (i = 0; i < BSpline.NumPoints; i++) {
        BSpline.CtrlPnts[i].x = BSpline.CtrlPnts[i].x * BSpline.CtrlPnts[i].w;
        BSpline.CtrlPnts[i].y = BSpline.CtrlPnts[i].y * BSpline.CtrlPnts[i].w;
    }
    memcpy(New, BSpline.CtrlPnts, sizeof(*New) * BSpline.NumPoints);
    for (p = 1; p <= BSpline.Degree; p++) {
        memcpy(Old, New, sizeof(New));
        for (i = j - BSpline.Degree + p; i <= j; i++) {
            k = (t - BSpline.KnotVector[i]) /
                (BSpline.KnotVector[i + BSpline.Degree - (p - 1)] - BSpline.KnotVector[i]);

            New[i].x = (1.f - k) * Old[MAX(i - 1, 0)].x + k * Old[i].x;
            New[i].y = (1.f - k) * Old[MAX(i - 1, 0)].y + k * Old[i].y;
            New[i].w = (1.f - k) * Old[MAX(i - 1, 0)].w + k * Old[i].w;
        }
    }

    New[j].x = New[j].x / New[j].w;
    New[j].y = New[j].y / New[j].w;
    return New[j];
}

PointStruct CurveEvalFirstDeriv(RealType t, CurveStruct Curve)
{
    PointStruct P,
        P0 = CurveEval(t, Curve),
        P1 = CurveEval(t - EPS, Curve);

    P = POINT(P0.x - P1.x,
              P0.y - P1.y);
    P.x = P.x / EPS;
    P.y = P.y / EPS;
    return P;
}

PointStruct CurveEvalSecondDeriv(RealType t, CurveStruct Curve)
{
    PointStruct P,
        P0 = CurveEval(t, Curve),
        P1 = CurveEval(t - DERIV_H, Curve),
        P2 = CurveEval(t - 2 * DERIV_H, Curve);

    P = POINT(P0.x - 2 * P1.x + P2.x,
              P0.y - 2 * P1.y + P2.y);

    P.x = P.x / (DERIV_H * DERIV_H);
    P.y = P.y / (DERIV_H * DERIV_H);

    return P;
}

PointStruct BezierEval(RealType t, CurveStruct Curve)
{
    IntType i, j;
    PointStruct Res[CURVE_MAX_ORDER];
    
    if (!Curve.NumPoints)
        return POINT_ZERO;

    for (i = 0; i < Curve.NumPoints; i++) {
        Curve.CtrlPnts[i].x = Curve.CtrlPnts[i].x * Curve.CtrlPnts[i].w;
        Curve.CtrlPnts[i].y = Curve.CtrlPnts[i].y * Curve.CtrlPnts[i].w;
    }

    memcpy(Res, Curve.CtrlPnts, sizeof(*Res) * Curve.NumPoints);
    for (j = 0; j < Curve.NumPoints - 1; j++) {
        for (i = 0; i < Curve.NumPoints - 1; i++) {
            Res[i].x = Res[i].x * (1.f - t) + Res[i + 1].x * t;
            Res[i].y = Res[i].y * (1.f - t) + Res[i + 1].y * t;
            Res[i].w = Res[i].w * (1.f - t) + Res[i + 1].w * t;
        }
    }

    Res[0].x = Res[0].x / Res[0].w;
    Res[0].y = Res[0].y / Res[0].w;
    return Res[0];
}

RealType BezierEval1D(RealType t, Curve1DStruct Curve)
{
    IntType i, j;
    RealType Res[CURVE_MAX_ORDER];

    memcpy(Res, Curve.CtrlPnts, sizeof(*Res) * Curve.NumPoints);
    for (j = 0; j < Curve.NumPoints - 1; j++)
        for (i = 0; i < Curve.NumPoints - 1; i++)
            Res[i] = (RealType)Res[i] * (1.f - t) + (RealType)Res[i + 1] * t;

    return Res[0];
}
