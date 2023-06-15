#include "curves.h"
#include "feel.h"
#include "proton.h"
#include "menu.h"
#include "renderer.h"
#include "palette.h"
#include "filePicker.h"

static PointStruct Root;

#define MENU_WIDTH (300)
#define MENU_EHGIHT (350)
#define MENU_ITEM_WIDTH (270)
#define MENU_ITEM_HEIGHT (30)
#define MENU_ITEM_OFFSET (50)

static const char
    *Strings[] = {
        "New Curve",
        "Load curve",
        "Toggle Ctrl Poly"
    };

void MenuUpdate(RealType Delta)
{


    if (!TIMER(GS_MENU))
        Root = Mouse;

    ProtonPushTransform();
    ProtonTranslate(Root);
    AABB_SET(AABB_MENU_ITEM_0, POINT(0, 0), POINT(MENU_WIDTH, MENU_EHGIHT));
    ProtonTranslate(POINT(20, 10));

    ProtonTranslate(POINT(0, MENU_ITEM_OFFSET));
    AABB_SET(AABB_MENU_ITEM_1,
            POINT(0, 0),
            POINT(MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT));

    ProtonTranslate(POINT(0, MENU_ITEM_OFFSET));
    AABB_SET(AABB_MENU_ITEM_2,
            POINT(0, 0),
            POINT(MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT));

    ProtonTranslate(POINT(0, MENU_ITEM_OFFSET));
    AABB_SET(AABB_MENU_ITEM_3,
            POINT(0, 0),
            POINT(MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT));

    ProtonPopTransform();

    if (Buttons[M1] && !LastButtons[M1] &&
        TIMER(AABB_CLICK + AABB_MENU_ITEM_0) == INACTIVE) {
        TIMER_STOP(GS_MENU);
    }

    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_1) != INACTIVE) {
        CurvesAddEmptyCurve();
        TIMER_STOP(GS_MENU);
    }

    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_2) != INACTIVE) {
        FilePickerSetRoot(Root);
        TIMER_START(GS_FILE_PICKER);
        TIMER_STOP(GS_MENU);
    }

    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_3) != INACTIVE) {
        CurvesToggleVisibility();
        TIMER_STOP(GS_MENU);
    }

}

void MenuDraw()
{
    IntType i;
    RealType t1, t2;
    PointStruct Top, Bottom, BorderTop, BorderBottom, p1;
    ColourStruct TextColour;

    t1 = ANIM_PARAM(TIMER(GS_MENU), UI_FADE_TIME);
    t1 = Casteljau1D(t1, EaseOut);
    Top = AABB[AABB_MENU_ITEM_0][0];
    Bottom = AABB[AABB_MENU_ITEM_0][1];
    p1 = POINT(Bottom.x, ANIM(t1, Top.y + 10, Bottom.y));

    BorderTop = POINT_ADD(POINT(10, 10), Top);
    BorderBottom = POINT_ADD(POINT(-10, -10), p1);
    ProtonSetFG(UI_BACKGROUND_COLOUR);
    ProtonFillRect(AABB[AABB_MENU_ITEM_0][0], p1);
    ProtonSetFG(UI_BORDER_COLOUR);
    ProtonDrawRect(BorderTop, BorderBottom);

    if (TIMER(GS_MENU) < UI_FADE_TIME)
        return;

    t1 = ANIM_PARAM(TIMER(GS_MENU) - UI_FADE_TIME, UI_TEXT_FADE_TIME);
    t1 = Casteljau1D(t1, EaseIn);

    for (i = 1; i < 4; i++) {
        t2 = ANIM_PARAM(TIMER(AABB_HOVER + AABB_MENU_ITEM_0 + i),
                        UI_TEXT_FADE_TIME);
        t2 = Casteljau1D(t2, EaseOut);


        TextColour = ColourAdd(ColourMult(UI_BACKGROUND_COLOUR, (1 - t1)),
                      ColourAdd(ColourMult(TEXT_NHOVERED_COLOUR, (1 - t2) * t1),
                      ColourMult(TEXT_HOVERED_COLOUR, t2 * t1)));

        ProtonSetFG(TextColour);
        ProtonDrawText(Strings[i - 1],
                        AABB[AABB_MENU_ITEM_0 + i][0]);
    }
}
