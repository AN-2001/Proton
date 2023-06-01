#include "feel.h"
#include "proton.h"
#include <string.h>

CurveStruct1D
    EaseIn = {{0.f, 0.f, 0.f, 1.f}, 4},
    EaseOut = {{0.f, 1.f, 1.f, 1.f}, 4},
    EaseInOut = {{0.f, 0.f, 1.f, 1.f}, 4},
    EaseOutIn = {{0.f, 1.f, 0.f, 1.f}, 4};

PointStruct Casteljau(RealType t, CurveStruct Curve)
{
    IntType i, j;
    PointStruct Res[CURVE_MAX_ORDER];

    memcpy(Res, Curve.CtrlPnts, sizeof(*Res) * Curve.Order);
    for (j = 0; j < Curve.Order - 1; j++) {
        for (i = 0; i < Curve.Order - 1; i++) {
            Res[i].x = (RealType)Res[i].x * (1.f - t) + (RealType)Res[i + 1].x * t;
            Res[i].y = (RealType)Res[i].y * (1.f - t) + (RealType)Res[i + 1].y * t;
        }
    }
    return Res[0];
}

RealType Casteljau1D(RealType t, CurveStruct1D Curve)
{
    IntType i, j;
    RealType Res[CURVE_MAX_ORDER];

    memcpy(Res, Curve.CtrlPnts, sizeof(*Res) * Curve.Order);
    for (j = 0; j < Curve.Order - 1; j++)
        for (i = 0; i < Curve.Order - 1; i++)
            Res[i] = (RealType)Res[i] * (1.f - t) + (RealType)Res[i + 1] * t;

    return Res[0];
}
