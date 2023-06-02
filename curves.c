#include "curves.h"
#include "feel.h"
#include "proton.h"
#include "renderer.h"

#define FADE_IN_TIME (50.f)

static PointStruct
    Pos = WIN_CENTRE;
static RealType
    Zoom = 1.f;

void CurvesUpdate(IntType Delta)
{
    IntType i;
    PointStruct p, w;
    IntType Rad;

    Zoom *= exp(MouseWheel * Delta * 1e-2);
    if (Buttons[M1]) {
        Pos.x = Pos.x - (Mouse.x - LastMouse.x) / Zoom;
        Pos.y = Pos.y - (Mouse.y - LastMouse.y) / Zoom;
    }

    RendererPushTransform();
    RendererTranslate(POINT(-Pos.x, -Pos.y));
    RendererScale(Zoom);
    RendererTranslate(WIN_CENTRE);
    
    srand(0);
    for (i = 0; i < 64; i++) {
        
        RendererSetFGA(rand() % 256, rand() % 256, rand() % 256, 0);
        p = POINT(rand() % WIN_WIDTH, rand() % WIN_HEIGHT);
        Rad = rand() % 30;

        w = RendererWorldToScreen(p);
        if (w.x < 0 || w.x >= WIN_WIDTH || w.y < 0 || w.y >= WIN_HEIGHT)
            continue;
        
        AABB_SET(AABB[i], POINT(p.x - Rad, p.y - Rad),
                          POINT(p.x + Rad, p.y + Rad));
    }

    RendererPopTransform();
}

void CurvesDraw()
{
    IntType i, Rad;
    RealType t, s;
    PointStruct p;

    t = ANIM_PARAM(GAME_STATE_TIMER(GAME_STATE_CURVES), FADE_IN_TIME);
    t = Casteljau1D(t, EaseIn);
    t = ANIM(t, 0, 255);
    RendererSetBG(0, 0, t);

    RendererPushTransform();
    RendererTranslate(POINT(-Pos.x, -Pos.y));
    RendererScale(Zoom);
    RendererTranslate(WIN_CENTRE);
    
    srand(0);
    for (i = 0; i < 64; i++) {
        s = 1.f;

        if (AABB_HIGHLIGHT_TIMER(i) != INACTIVE) {
            s = ANIM_PARAM(AABB_HIGHLIGHT_TIMER(i), 10.f);
            s = Casteljau1D(s, EaseOutIn);
            s = ANIM(s, 1.f, 0.5f);
        }

        RendererSetFGA(rand() % 256 * s, rand() % 256 * s, rand() % 256 * s, t);
        p = POINT(rand() % WIN_WIDTH, rand() % WIN_HEIGHT);
        Rad = rand() % 30;

        RendererFillCircle(p, Rad);
    }

    RendererPopTransform();

    RendererSetFG(0, 255, 0);
    for (i = 0; i < 64; i++)
        RendererDrawRectangle(AABB[i][0], AABB[i][1]);

}
