#include "curves.h"
#include "feel.h"
#include "proton.h"
#include "renderer.h"
#define JUMP_TICK_COUNT (50.f)
#define SQUISH_TICK_COUNT (30.f)

static CurveStruct1D
    JumpCurve = {{0.f, 1.f, 1.f, 0.f}, 4},
    RotationCurve = {{0.f, 1.477, 1.093, 1.f}, 4},
    SquishCurve = {{0.f, 0.5f, 0.5f, 0.f}, 4};
    
void CurvesUpdate(IntType Delta)
{
    if (Keys['j'] && !LastKeys['j'] &&
            TIMER(GP_0) == INACTIVE &&
            TIMER(GP_1) == INACTIVE)
        TIMER_START(GP_0);

    if (TIMER(GP_0) == JUMP_TICK_COUNT) {
        TIMER_STOP(GP_0);
        TIMER_START(GP_1);
    }

    if (TIMER(GP_1) == SQUISH_TICK_COUNT)
        TIMER_STOP(GP_1);

}


void CurvesDraw()
{
    RealType y, Ang, Squish, SquishH;


    y = ANIM_PARAM(TIMER(GP_0), JUMP_TICK_COUNT);
    Ang = ANIM_PARAM(TIMER(GP_0), JUMP_TICK_COUNT);
    Squish = ANIM_PARAM(TIMER(GP_1), SQUISH_TICK_COUNT);

    y = Casteljau1D(y, JumpCurve);
    Ang = Casteljau1D(Ang, RotationCurve);
    Squish = Casteljau1D(Squish, SquishCurve);

    Ang = ANIM(Ang, 0, M_PI / 2.f);
    y = ANIM(y, WIN_CENTRE.y, WIN_CENTRE.y - 100);
    SquishH = ANIM(Squish, 0, 30);
    Squish = ANIM(Squish, -40, 10);

    /* Drawing code.                                                          */
    ProtonSetFG(255, 255, 255);
    ProtonPushTransform();
    ProtonRotate(Ang);
    ProtonTranslate(POINT(WIN_CENTRE.x, y));

    ProtonDrawLine(POINT(-40 - SquishH, Squish),
                   POINT(40 + SquishH, Squish));
    ProtonDrawLine(POINT(40 + SquishH, Squish),
                   POINT(40 + SquishH, 40));
    ProtonDrawLine(POINT(40 + SquishH, 40),
                   POINT(-40 - SquishH, 40));
    ProtonDrawLine(POINT(-40 - SquishH, 40),
                   POINT(-40 - SquishH, Squish));

    ProtonPopTransform();
}
