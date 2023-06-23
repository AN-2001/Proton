#include "feel.h"
#include "proton.h"
#include "inputPrompt.h"
#include "renderer.h"
#include <memory.h>
#include "palette.h"

#define ENTER (13)
#define BACKSAPCE (8)
#define ESCAPE (27)

char TextInput[BUFF_SIZE];

static IntType
    CurrentChar = 0;

void InputPromptUpdate(RealType Delta)
{
    IntType i;

    if (!TIMER(GS_INPUT_PROMPT)) {
        memset(TextInput, 0, sizeof(TextInput));
        CurrentChar = -1;
    }

    for (i = 0; i < KEYS_TOTAL; i++) {
        if (Keys[i] && !LastKeys[i]) {
            if (i == ENTER) {
                TIMER_STOP(GS_INPUT_PROMPT);
                return;
            }

            if (i == BACKSAPCE) {
                if (CurrentChar >= 0) {
                    TextInput[CurrentChar--] = '\0';
                    CurrentChar = MAX(CurrentChar, -1);
                }
                return;
            }

            if (i == ESCAPE) {
                memset(TextInput, 0, sizeof(TextInput));
                TIMER_STOP(GS_INPUT_PROMPT);
                return;
            }

            TextInput[++CurrentChar] = i;
        }
    }

}

void InputPromptDraw()
{
    PointStruct p,
        TextPosition;
    RealType
        t = ANIM_PARAM(TIMER(GS_INPUT_PROMPT), UI_FADE_TIME);

    t = BezierEval1D(t, EaseIn);
    p.y = WIN_CENTRE.y + 20;
    p.x = ANIM(t, 0, WIN_WIDTH);

    ProtonSetFG(UI_BACKGROUND_COLOUR);
    ProtonFillRect(POINT(0, WIN_CENTRE.y - 20), p);

    ProtonSetFG(UI_BORDER_COLOUR);
    ProtonDrawRect(POINT(0, WIN_CENTRE.y - 20), p);

    TextPosition.y = WIN_CENTRE.y - 10;
    TextPosition.x = WIN_CENTRE.x - (CurrentChar + 1) * 5 - 20;

    ProtonSetFG(TEXT_HOVERED_COLOUR);
    if (TextInput[0]) {
        ProtonSetFG(TEXT_HOVERED_COLOUR);
        ProtonDrawText(TextInput, TextPosition);
    }
}

