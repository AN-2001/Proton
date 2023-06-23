#include "curves.h"
#include "feel.h"
#include "proton.h"
#include "menu.h"
#include "renderer.h"
#include "palette.h"
#include "filePicker.h"
#include "inputPrompt.h"

static PointStruct Root;

#define MENU_WIDTH (320)
#define MENU_EHGIHT (350)
#define MENU_ITEM_WIDTH (270)
#define MENU_ITEM_HEIGHT (30)
#define MENU_ITEM_OFFSET (50)
#define MENU_MAX_ITEMS (4)

static IntType
    FadeStart = 0,
    MenuState = 0;
static BoolType
    WaitingForInput = FALSE;

static void HandleState0();
static void HandleState1();
static void HandleState2();
static void HandleState3();
static void HandleState4();
static void HandleState5();
static const char
    *Strings[][MENU_MAX_ITEMS] = {
        {"New Curve",
         "Load/save curve",
         "Edit B-spline",
         "View settings"},

        {"New Bezier",
         "New B-Spline",
         "Connect curves",
         NULL},

        {"Edit knot vector",
         "Set degree",
         "Set end conditions",
         NULL},

        {"Uniform floating",
         "Uniform open",
         "floating",
         "open"},

        {"Toggle tangnet",
         "Toggle normal",
         "Toggle osculating circle",
         "Toggle control polygon"
        },

        {"C0 continuity",
         "C1 continuity",
         "G1 continuity",
         NULL}

    };

static void
    (*StateHandlers[])() = {
            HandleState0,
            HandleState1,
            HandleState2,
            HandleState3,
            HandleState4,
            HandleState5,
    };
    
void MenuUpdate(RealType Delta)
{
    IntType Degree;

    if (TIMER(GS_INPUT_PROMPT) != INACTIVE)
        return;

    if (!TIMER(GS_MENU)) {
        MenuState = 0;
        FadeStart = UI_FADE_TIME;
        Root = Mouse;
    }

    if (WaitingForInput) {
        if (sscanf(TextInput, "%d", &Degree) != 1) {
            TIMER_START(GS_INPUT_PROMPT);
            WaitingForInput = TRUE;
        } else {
            PickedCurve -> Degree = Degree;
            PickedCurve -> NeedsRefresh = TRUE;
            PickedCurve -> NeedsKnots = TRUE;
            WaitingForInput = FALSE;
        }
    }

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

    ProtonTranslate(POINT(0, MENU_ITEM_OFFSET));
    AABB_SET(AABB_MENU_ITEM_4,
            POINT(0, 0),
            POINT(MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT));

    ProtonPopTransform();


    if (Buttons[M1] && !LastButtons[M1] &&
            TIMER(AABB_CLICK + AABB_MENU_ITEM_0) == INACTIVE) {
        TIMER_STOP(GS_MENU);
        return;
    }

    if ((Buttons[M1] && !LastButtons[M1]) || 
            (Buttons[M2] && !LastButtons[M2]))
        StateHandlers[MenuState]();



}

void MenuDraw()
{
    IntType i;
    RealType t1, t2;
    PointStruct Top, Bottom, BorderTop, BorderBottom, p1;
    ColourStruct TextColour;

    t1 = ANIM_PARAM(TIMER(GS_MENU), UI_FADE_TIME);
    t1 = BezierEval1D(t1, EaseOut);
    Top = AABB[AABB_MENU_ITEM_0][0];
    Bottom = AABB[AABB_MENU_ITEM_0][1];
    p1 = POINT(Bottom.x, ANIM(t1, Top.y + 20, Bottom.y));

    BorderTop = POINT_ADD(POINT(10, 10), Top);
    BorderBottom = POINT_ADD(POINT(-10, -10), p1);
    ProtonSetFG(UI_BACKGROUND_COLOUR);
    ProtonFillRect(AABB[AABB_MENU_ITEM_0][0], p1);
    ProtonSetFG(UI_BORDER_COLOUR);
    ProtonDrawRect(BorderTop, BorderBottom);

    t1 = ANIM_PARAM(TIMER(GS_MENU) - FadeStart, UI_TEXT_FADE_TIME);
    t1 = BezierEval1D(t1, EaseIn);

    if (TIMER(GS_MENU) < FadeStart)
        return;

    for (i = 1; Strings[MenuState][i - 1] && i < MENU_MAX_ITEMS + 1; i++) {
        t2 = ANIM_PARAM(TIMER(AABB_HOVER + AABB_MENU_ITEM_0 + i),
                        UI_TEXT_FADE_TIME);
        t2 = BezierEval1D(t2, EaseOut);


        TextColour = ColourAdd(ColourMult(UI_BACKGROUND_COLOUR, (1 - t1)),
                      ColourAdd(ColourMult(TEXT_NHOVERED_COLOUR, (1 - t2) * t1),
                      ColourMult(TEXT_HOVERED_COLOUR, t2 * t1)));

        ProtonSetFG(TextColour);
        ProtonDrawText(Strings[MenuState][i - 1],
                        AABB[AABB_MENU_ITEM_0 + i][0]);
    }
}

static void HandleState0()
{
    if (Buttons[M2] && !LastButtons[M2]) {
        TIMER_STOP(GS_MENU);
    }

    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_1) != INACTIVE) {
        MenuState = 1;
        FadeStart = TIMER(GS_MENU);
    }

    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_2) != INACTIVE) {
        FilePickerSetRoot(Root);
        TIMER_START(GS_FILE_PICKER);
        TIMER_STOP(GS_MENU);
    }
    if (PickedCurve && PickedCurve -> Type == CURVE_TYPE_BSPLINE
            && TIMER(AABB_CLICK + AABB_MENU_ITEM_3) != INACTIVE) {
        MenuState = 2;
        FadeStart = TIMER(GS_MENU);

    }
    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_4) != INACTIVE) {
        MenuState = 4;
        FadeStart = TIMER(GS_MENU);
    }
}

void HandleState1()
{
    IntType Degree;

    if (Buttons[M2] && !LastButtons[M2]) {
        MenuState = 0;
        FadeStart = TIMER(GS_MENU);
    }
    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_1) != INACTIVE) {
        CurvesAddEmptyBezier();
        TIMER_STOP(GS_MENU);
    }
    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_2) != INACTIVE) {
        CurvesAddEmptyBspline();
        TIMER_STOP(GS_MENU);
    }

    if (PickedCurve && 
            TIMER(AABB_CLICK + AABB_MENU_ITEM_3) != INACTIVE) {
        MenuState = 5;
        FadeStart = TIMER(GS_MENU);
    }
}

void HandleState2()
{
    if (Buttons[M2] && !LastButtons[M2]) {
        MenuState = 0;
        FadeStart = TIMER(GS_MENU);
    }
    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_1) != INACTIVE) {
        TIMER_START(GS_KNOT_EDITOR);
        TIMER_STOP(GS_MENU);
    }
    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_2) != INACTIVE) {
        WaitingForInput = TRUE;
        TIMER_START(GS_INPUT_PROMPT);
    }
    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_3) != INACTIVE) {
        MenuState = 3;
        FadeStart = TIMER(GS_MENU);
    }
}

void HandleState3()
{
    if (Buttons[M2] && !LastButtons[M2]) {
        MenuState = 2;
        FadeStart = TIMER(GS_MENU);
    }
    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_1) != INACTIVE) {
        PickedCurve -> EndConditions = END_CONDITIONS_UNIFORM_FLOATING;
        PickedCurve -> NeedsKnots = TRUE;
        PickedCurve -> NeedsRefresh = TRUE;
    }
    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_2) != INACTIVE) {
        PickedCurve -> EndConditions = END_CONDITIONS_UNIFORM_OPEN;
        PickedCurve -> NeedsKnots = TRUE;
        PickedCurve -> NeedsRefresh = TRUE;
    }
    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_3) != INACTIVE) {
        PickedCurve -> EndConditions = END_CONDITIONS_FLOATING;
        PickedCurve -> NeedsKnots = TRUE;
        PickedCurve -> NeedsRefresh = TRUE;
    }
    if (PickedCurve && TIMER(AABB_CLICK + AABB_MENU_ITEM_4) != INACTIVE) {
        PickedCurve -> EndConditions = END_CONDITIONS_OPEN;
        PickedCurve -> NeedsKnots = TRUE;
        PickedCurve -> NeedsRefresh = TRUE;
    }
}

void HandleState4()
{
    if (Buttons[M2] && !LastButtons[M2]) {
        MenuState = 0;
        FadeStart = TIMER(GS_MENU);
    }
    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_1) != INACTIVE) {
        CurvesToggleTangent();
    }
    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_2) != INACTIVE) {
        CurvesToggleNormal();
    }
    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_3) != INACTIVE) {
        CurvesToggleOsculatingCircle();

    }
    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_4) != INACTIVE) {
        CurvesToggleControlPolygon();
    }
}

void HandleState5()
{
    if (Buttons[M2] && !LastButtons[M2]) {
        MenuState = 1;
        FadeStart = TIMER(GS_MENU);
    }
    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_1) != INACTIVE) {
        CurvesConnectCurves(CONNECTION_C0);
        TIMER_STOP(GS_MENU);
    }
    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_2) != INACTIVE) {
        CurvesConnectCurves(CONNECTION_C1);
        TIMER_STOP(GS_MENU);
    }
    if (TIMER(AABB_CLICK + AABB_MENU_ITEM_3) != INACTIVE) {
        CurvesConnectCurves(CONNECTION_G1);
        TIMER_STOP(GS_MENU);
    }
}
