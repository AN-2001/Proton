#include "menu.h"
#include "proton.h"
#include "renderer.h"


#define MENU_WIDTH (200)
#define MENU_HEIGHT (180)
#define MENU_ITEM_WIDTH (MENU_WIDTH - 20)
#define MENU_ITEM_HEIGHT (50)
#define MENU_ITEM_OFFSET (55)
#define MENU_OFFSET (10)
#define TEXT_OFFSET_X (10)
#define TEXT_OFFSET_Y (15)

static PointStruct Root;

void MenuUpdate(IntType Delta)
{
    (void)Delta;
    IntType
        TickDelta = Tick - GameStateTicks[GAME_STATE_MENU];


    if (MouseKeys[M2] && !LastMouseKeys[M2]) {
        Root = Mouse;
        GameStateTicks[GAME_STATE_MENU] = Tick;
    }

    if (!TickDelta || (MouseKeys[M2] && !LastMouseKeys[M2])) {
        Root = Mouse;
        RendererPushTransform(Root, 1.f); 
        AABB_SET(AABB[AABB_TYPE_MENU_ITEM_0], POINT_ZERO, POINT(MENU_WIDTH, MENU_HEIGHT));
        RendererPushTransform(POINT(MENU_OFFSET, MENU_OFFSET), 1.f); 
        AABB_SET(AABB[AABB_TYPE_MENU_ITEM_1], POINT_ZERO, POINT(MENU_WIDTH, MENU_HEIGHT));
        RendererPushTransform(POINT(MENU_OFFSET, MENU_OFFSET), 1.f); 
        AABB_SET(AABB[AABB_TYPE_MENU_ITEM_2], POINT_ZERO, POINT(MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT));

        RendererPushTransform(POINT(0, MENU_ITEM_OFFSET), 1.f); 
        AABB_SET(AABB[AABB_TYPE_MENU_ITEM_3], POINT_ZERO, POINT(MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT));

        RendererPushTransform(POINT(0, MENU_ITEM_OFFSET), 1.f); 
        AABB_SET(AABB[AABB_TYPE_MENU_ITEM_4], POINT_ZERO, POINT(MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT));

        RendererPopTransform();
        RendererPopTransform();
        RendererPopTransform();
                    
        RendererPopTransform();
        RendererPopTransform();
    }

    if (AABBClickTicks[AABB_TYPE_MENU_ITEM_4] != INACTIVE) {
        Debug = !Debug;
        GameState[GAME_STATE_MENU] = FALSE;
    }

    if (AABBClickTicks[AABB_TYPE_MENU_ITEM_3] != INACTIVE) {
        GameState[GAME_STATE_MENU] = FALSE;
        GameState[GAME_STATE_FILE_PICKER] = TRUE;
    }

    if (MouseKeys[M1] && !LastMouseKeys[M1] &&
            AABBHighlightTicks[AABB_TYPE_MENU_ITEM_0] == INACTIVE) {
        GameState[GAME_STATE_MENU] = FALSE;
    }

}

void MenuDraw()
{
    PointStruct p1, p2;;
    
    RendererSetFGColour(0, 255, 255);
    RendererFillRectangle(AABB[AABB_TYPE_MENU_ITEM_0][0],
                          AABB[AABB_TYPE_MENU_ITEM_0][1]);

    RendererSetFGColour(0, 0, 255);
    RendererFillRectangle(AABB[AABB_TYPE_MENU_ITEM_1][0],
                          AABB[AABB_TYPE_MENU_ITEM_1][1]);

    p1 = AABB[AABB_TYPE_MENU_ITEM_1][0];
    p2 = AABB[AABB_TYPE_MENU_ITEM_1][1];
    RendererSetFGColour(255, 255, 255);
    RendererDrawRectangle(POINT(p1.x + 5, p1.y + 5),
                          POINT(p2.x - 5, p2.y - 5));

    RendererPushTransform(AABB[AABB_TYPE_MENU_ITEM_2][0], 1.f);
    if (AABBHighlightTicks[AABB_TYPE_MENU_ITEM_2] != INACTIVE)
        RendererSetFGColour(0, 100, 255);
    else
        RendererSetFGColour(0, 100, 200);
    RendererFillRectangle(POINT_ZERO,
                          POINT(MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT));
    RendererSetFGColour(0, 0, 0);
    RendererDrawText("New curve", POINT(TEXT_OFFSET_X, TEXT_OFFSET_Y));
    RendererPopTransform();

    RendererPushTransform(AABB[AABB_TYPE_MENU_ITEM_3][0], 1.f);
    if (AABBHighlightTicks[AABB_TYPE_MENU_ITEM_3] != INACTIVE)
        RendererSetFGColour(0, 100, 255);
    else
        RendererSetFGColour(0, 100, 200);
    RendererFillRectangle(POINT_ZERO,
                          POINT(MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT));
    RendererSetFGColour(0, 0, 0);
    RendererDrawText("Load curve", POINT(TEXT_OFFSET_X, TEXT_OFFSET_Y));
    RendererPopTransform();

    RendererPushTransform(AABB[AABB_TYPE_MENU_ITEM_4][0], 1.f);
    if (AABBHighlightTicks[AABB_TYPE_MENU_ITEM_4] != INACTIVE)
        RendererSetFGColour(0, 100, 255);
    else
        RendererSetFGColour(0, 100, 200);
    RendererFillRectangle(POINT_ZERO,
                          POINT(MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT));
    RendererSetFGColour(0, 0, 0);
    RendererDrawText("Debug", POINT(TEXT_OFFSET_X, TEXT_OFFSET_Y));
    RendererPopTransform();


}
