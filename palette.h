#ifndef PALETTE_H
#define PALETTE_H
#include "proton.h"

#define HEX2COL(NUM) \
    COLOUR(((unsigned)NUM >> 16) & 0xff, \
           ((unsigned)NUM >> 8)  & 0xff, \
           ((unsigned)NUM >> 0)  & 0xff, \
           255)

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
              

#define BACKGROUND_COLOUR           HEX2COL(0x242837)
#define MAJOR_GRID_COLOUR           HEX2COL(0x403F4C)
#define MINOR_GRID_COLOUR           HEX2COL(0x2C2B3C)
#define CURVE_COLOUR                HEX2COL(0xFFFFFF)
#define CONTROL_POLY_COLOUR         HEX2COL(0xB76D68)
#define CONTROL_POINT_COLOUR        HEX2COL(0xC19595)
#define PICKED_CONTROL_POINT_COLOUR HEX2COL(0xFFFFFF)
#define UI_BACKGROUND_COLOUR        HEX2COL(0x242837)
#define UI_BORDER_COLOUR            HEX2COL(0xFFFFFF)
#define TEXT_NHOVERED_COLOUR        HEX2COL(0x999999)
#define TEXT_FILE_NHOVERED_COLOUR   HEX2COL(0x888888)
#define TEXT_DIR_NHOVERED_COLOUR    HEX2COL(0xEEEEEE)
#define TEXT_HOVERED_COLOUR         HEX2COL(0x727FB0)

#define UI_FADE_TIME (20.f)
#define UI_TEXT_FADE_TIME (10.f)

#endif
