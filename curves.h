#ifndef CURVES_H
#define CURVES_H
#include "feel.h"
#include "proton.h"

#define CURVES_TOTAL (1024)

extern CurveStruct *PickedCurve;

typedef enum ConnectionEnum {
    CONNECTION_C0,
    CONNECTION_C1,
    CONNECTION_G1
} ConnectionEnum;

void CurvesUpdate(RealType Delta);
void CurvesAddEmptyBezier();
void CurvesAddEmptyBspline();
void CurvesAddFromPath(const char *Path);
void CurvesToggleTangent();
void CurvesToggleNormal();
void CurvesToggleOsculatingCircle();
void CurvesToggleControlPolygon();
void CurvesConnectCurves(ConnectionEnum ConnectionType);
void CurvesDraw();

#endif /* CURVES_H */
