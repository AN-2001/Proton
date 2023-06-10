#ifndef FEEL_H
#define FEEL_H
#include "proton.h"

#define CURVE_MAX_ORDER (64)

typedef struct CurveStruct {
    PointStruct CtrlPnts[CURVE_MAX_ORDER];
    IntType Order;
} CurveStruct;

typedef struct Curve1DStruct {
    RealType CtrlPnts[CURVE_MAX_ORDER];
    IntType Order;
} Curve1DStruct;

extern Curve1DStruct EaseIn;
extern Curve1DStruct EaseOut;
extern Curve1DStruct EaseInOut;
extern Curve1DStruct EaseOutIn;

PointStruct Casteljau(RealType t, CurveStruct Curve);
RealType Casteljau1D(RealType t, Curve1DStruct Curve);

#endif /* FEEL_H */
