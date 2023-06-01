#ifndef FEEL_H
#define FEEL_H
#include "proton.h"

#define CURVE_MAX_ORDER (64)

typedef struct CurveStruct {
    PointStruct CtrlPnts[CURVE_MAX_ORDER];
    IntType Order;
} CurveStruct;

typedef struct CurveStruct1D {
    RealType CtrlPnts[CURVE_MAX_ORDER];
    IntType Order;
} CurveStruct1D;

extern CurveStruct1D EaseIn;
extern CurveStruct1D EaseOut;
extern CurveStruct1D EaseInOut;
extern CurveStruct1D EaseOutIn;

PointStruct Casteljau(RealType t, CurveStruct Curve);
RealType Casteljau1D(RealType t, CurveStruct1D Curve);

#endif /* FEEL_H */
