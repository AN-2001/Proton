#include "knotEditor.h"
#include "feel.h"
#include "palette.h"
#include "proton.h"
#include "renderer.h"
#include "curves.h"

#define EDITOR_WIDTH (WIN_WIDTH - 100)
#define EIDTOR_HEIGHT (WIN_HEIGHT / 2.f - 100)

static IntType
    PickedKnot = INACTIVE;

int RealCmp(const void *R1, const void *R2);

void knotEditorUpdate(RealType Delta)
{
    IntType i,
        NumKnots = BSplineNumKnots(*PickedCurve);
    PointStruct p;

    AABB_SET(AABB_MENU_ITEM_0,
             POINT(10, WIN_HEIGHT / 2.f + 10),
             POINT(WIN_WIDTH - 10, WIN_HEIGHT - 10));

    AABB_SET(AABB_MENU_ITEM_1,
             POINT(50, WIN_HEIGHT / 2.f + 50),
             POINT(WIN_WIDTH - 50, WIN_HEIGHT - 50));

    if (Buttons[M1] && !LastButtons[M1] &&
            TIMER(AABB_CLICK + AABB_MENU_ITEM_0) == INACTIVE)
        TIMER_STOP(GS_KNOT_EDITOR);

    if (PickedKnot == INACTIVE) {
        for (i = 0; i < NumKnots; i++) {
            if (TIMER(AABB_CLICK + AABB_MENU_TOTAL + i) != INACTIVE) {
                PickedKnot = i;
                break;
            }
        }
    }

    if (!Buttons[M1])
        PickedKnot = INACTIVE;

    ProtonPushTransform();
    ProtonTranslate(AABB[AABB_MENU_ITEM_1][0]);

    for (i = 0; i < NumKnots; i++) {
        p = POINT(PickedCurve -> KnotVector[i] * EDITOR_WIDTH, 
                  EIDTOR_HEIGHT / 2.f);

        AABB_SET(AABB_MENU_TOTAL + i,
                 POINT(p.x - 10, p.y - 10),
                 POINT(p.x + 10, p.y + 10));
    }


    if (PickedKnot != INACTIVE) {
        PickedCurve -> KnotVector[PickedKnot] = MAX(MIN(ProtonScreenToWorld(Mouse).x / EDITOR_WIDTH, 1.f), 0.f);
        
        qsort(PickedCurve -> KnotVector, NumKnots, sizeof(PickedCurve -> KnotVector[0]), RealCmp);
        PickedCurve -> NeedsRefresh = TRUE;
    }

    ProtonPopTransform();

}

void knotEditorDraw()
{
    IntType i,
        NumKnots = BSplineNumKnots(*PickedCurve);
    RealType
        t = ANIM_PARAM(TIMER(GS_KNOT_EDITOR), UI_FADE_TIME);
    PointStruct p;
    ColourStruct c;

    t = BezierEval1D(t, EaseOut);
    p = AABB[AABB_MENU_ITEM_0][1];
    p.y = ANIM(t, AABB[AABB_MENU_ITEM_0][0].y, AABB[AABB_MENU_ITEM_0][1].y);

    c = UI_BACKGROUND_COLOUR;
    ProtonSetFG(c);

    ProtonFillRect(AABB[AABB_MENU_ITEM_0][0], p);

    c = UI_BORDER_COLOUR;
    ProtonSetFG(c);

    ProtonDrawRect(POINT_ADD(AABB[AABB_MENU_ITEM_0][0], POINT(10, 10)),
                   POINT_ADD(p, POINT(-10, -10)));


    t = ANIM_PARAM(TIMER(GS_KNOT_EDITOR) - UI_FADE_TIME, UI_TEXT_FADE_TIME);
    t = BezierEval1D(t, EaseOut);

    c = UI_FG_COLOUR;
    c.a = t * UI_FG_COLOUR.a;
    ProtonSetFG(c);

    ProtonFillRect(AABB[AABB_MENU_ITEM_1][0], AABB[AABB_MENU_ITEM_1][1]);

    c = COLOUR_WHITE;
    c.a = t * COLOUR_WHITE.a;
    ProtonSetFG(c);

    ProtonPushTransform();
    ProtonTranslate(AABB[AABB_MENU_ITEM_1][0]);
    ProtonDrawLine(POINT(0, EIDTOR_HEIGHT / 2.f),
                   POINT(EDITOR_WIDTH, EIDTOR_HEIGHT / 2.f));


    for (i = 0; i <= 10; i++) {
        ProtonDrawLine(POINT(EDITOR_WIDTH * (i / 10.f), EIDTOR_HEIGHT / 2.f + 10),
                       POINT(EDITOR_WIDTH * (i / 10.f), EIDTOR_HEIGHT / 2.f - 10));
                        
    }

    c = KNOT_COLOUR;
    c.a = t * KNOT_COLOUR.a;
    ProtonSetFG(c);

    for (i = 0; i < NumKnots; i++) {
        ProtonFillCircle(POINT(PickedCurve -> KnotVector[i] * EDITOR_WIDTH,
                               EIDTOR_HEIGHT / 2.f), 10);
    }


    ProtonPopTransform();
}

int RealCmp(const void *R1, const void *R2)
{
    RealType 
        Num1 = *(RealType*)R1,
        Num2 = *(RealType*)R2;

    return Num1 > Num2;
}
