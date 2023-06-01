#ifndef TRANSFORM_H
#define TRANSFORM_H
#include "proton.h"

#define TRANS_APPLY_POINT(T, P) TransformApply(T, P, TRUE)
#define TRANS_APPLY_VEC(T, V) TransformApply(T, V, FALSE)

static inline PointStruct TransformApply(MatType Transform,
                                  PointStruct Point,
                                  BoolType IsPoint)
{
    return POINT(Point.x * Transform[0][0] + Transform[0][2] * IsPoint,
                 Point.y * Transform[1][1] + Transform[1][2] * IsPoint);
}

static inline void TransformMult(MatType Left, MatType Right, MatType Dest)
{
    MatType Ret;

    Ret[0][0] = Left[0][0] * Right[0][0]; 
    Ret[1][1] = Left[1][1] * Right[1][1]; 
    Ret[0][2] = Right[0][2] * Left[0][0] + Left[0][2]; 
    Ret[1][2] = Right[1][2] * Left[1][1] + Left[1][2]; 

    Dest[0][0] = Ret[0][0];
    Dest[1][1] = Ret[1][1];
    Dest[0][2] = Ret[0][2];
    Dest[1][2] = Ret[1][2];
}

static inline void TransformInvert(MatType Transform, MatType Inverse)
{
    Inverse[0][0] = 1.f / Transform[0][0];
    Inverse[1][1] = 1.f / Transform[1][1];
    Inverse[0][2] = -Transform[0][2] / Transform[0][0];
    Inverse[1][2] = -Transform[1][2] / Transform[1][1];
}

#endif /* TRANSFORM_H */
