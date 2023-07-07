#ifndef PALETTE_H
#define PALETTE_H
#include "proton.h"

#define HEX2COL(NUM) \
    COLOUR(((unsigned)NUM >> 24) & 0xff, \
           ((unsigned)NUM >> 16) & 0xff, \
           ((unsigned)NUM >> 8)  & 0xff, \
           ((unsigned)NUM >> 0)  & 0xff)

static inline ColourStruct ColourMult(ColourStruct C, RealType t)
{
    return COLOUR(C.r * t * 255, C.g * t * 255, C.b * t * 255, C.a * t * 255);
}

static inline ColourStruct ColourAdd(ColourStruct C1, ColourStruct C2)
{
    return COLOUR((C1.r + C2.r) * 255,
                  (C1.g + C2.g) * 255,
                  (C1.b + C2.b) * 255,
                  (C1.a + C2.a) * 255);
}
              
#define COLOUR_WHITE                COLOUR(255, 255, 255, 255)
#define TANGENT_COLOUR              COLOUR(255, 0, 0, 255)
#define NORMAL_COLOUR               COLOUR(0, 255, 0, 255)
#define OSCULATING_CIRCLE_COLOUR    COLOUR(0, 0, 255, 255)
#define WEIGHT_RAD_COLOUR           COLOUR(255, 0, 0, 255)
#define BACKGROUND_COLOUR           HEX2COL(0x242837ff)
#define MAJOR_GRID_COLOUR           HEX2COL(0x403F4Cff)
#define MINOR_GRID_COLOUR           HEX2COL(0x2C2B3Cff)
#define CURVE_COLOUR                HEX2COL(0xFFFFFFff)
#define CONTROL_POLY_COLOUR         HEX2COL(0xFF9700ff)
#define CONTROL_POINT_COLOUR        HEX2COL(0xFF9700ff)
#define KNOT_COLOUR                 HEX2COL(0xFF9700ff)
#define PICKED_CONTROL_POINT_COLOUR HEX2COL(0xFFFFFFff)
#define UI_BACKGROUND_COLOUR        HEX2COL(0x24283799)
#define UI_FG_COLOUR                HEX2COL(0x49517099)
#define UI_BORDER_COLOUR            HEX2COL(0xFFFFFFFF)
#define TEXT_NHOVERED_COLOUR        HEX2COL(0x999999ff)
#define TEXT_FILE_NHOVERED_COLOUR   HEX2COL(0x888888ff)
#define TEXT_DIR_NHOVERED_COLOUR    HEX2COL(0xEEEEEEff)
#define TEXT_HOVERED_COLOUR         HEX2COL(0x727FB0ff)

#define UI_FADE_TIME (35.f)
#define UI_TEXT_FADE_TIME (10.f)

#endif
