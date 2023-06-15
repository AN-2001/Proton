#include "curves.h"
#include "proton.h"
#include "renderer.h"
#include "dirent.h"
#include <linux/limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "feel.h"
#include "palette.h"

#define FILE_PICKER_WIDTH (300)
#define FILE_PICKER_HEIGHT (350)
#define FILE_PICKER_ITEMS_OFFSET_X (20)
#define FILE_PICKER_ITEMS_OFFSET_Y (20)
#define FILE_PICKER_ITEM_OFFSET_Y (40)
#define FILE_PICKER_ITEM_WIDTH (270)
#define FILE_PICKER_ITEM_HEIGHT (30)
#define FILE_PICKER_ITEMS_TOTAL (8)

static char
    CurrentDir[PATH_MAX] = {'.', '\0'};
static struct FileInfo{
    char Name[PATH_MAX];
    int Type;
} *LoadedFiles = NULL;
static IntType
    LoadedFilesSize = 0;
static PointStruct
    Root = POINT_ZERO;
static IntType
    Scroll = 0;

static void LoadFiles();
static void ChangeCurrentPath(const char *Picked);
static size_t CountFiles(DIR *Dir);
static int Compare(const void *p1, const void *p2);

void FilePickerUpdate(RealType Delta)
{
    IntType i, k;

    if (Buttons[M1] && !LastButtons[M1] &&
            TIMER(AABB_HOVER + AABB_MENU_ITEM_4) == INACTIVE) {
        TIMER_STOP(GS_FILE_PICKER);
    }

    if (!TIMER(GS_FILE_PICKER)) {
        CurrentDir[0] = '.';
        CurrentDir[1] = '\0';
        Scroll = 0;
        LoadFiles();
    }

    Scroll -= (MouseWheel < 0 ? -1 : 1) * (MouseWheel != 0);
    Scroll = MAX(MIN(Scroll, LoadedFilesSize - 8), 0);

    if (Buttons[M1] && !LastButtons[M1]) {
        for (i = 0; i < LoadedFilesSize - Scroll; i++) {
            k = AABB_MENU_ITEM_4 + i + 1; 
            if (TIMER(AABB_CLICK + k) != INACTIVE &&
                    LoadedFiles[i + Scroll].Type)
                ChangeCurrentPath(LoadedFiles[i + Scroll].Name);
        }
    }

    ProtonPushTransform();
    ProtonTranslate(Root);
    AABB_SET(AABB_MENU_ITEM_4, POINT_ZERO, POINT(FILE_PICKER_WIDTH, FILE_PICKER_HEIGHT));

    ProtonTranslate(POINT(FILE_PICKER_ITEMS_OFFSET_X,
                          FILE_PICKER_ITEMS_OFFSET_Y));

    for (i = 0; i < FILE_PICKER_ITEMS_TOTAL; i++) {
        k = AABB_MENU_ITEM_4 + i + 1; 
        AABB_SET(k,
                 POINT_ZERO,
                 POINT(FILE_PICKER_ITEM_WIDTH, FILE_PICKER_ITEM_HEIGHT));
        ProtonTranslate(POINT(0, FILE_PICKER_ITEM_OFFSET_Y));
    }

    ProtonPopTransform();
}

void FilePickerDraw()
{
    IntType i, k;
    RealType t1, t2;
    PointStruct p, TopLeft, BottomRight, BorderTop, BorderBottom;
    ColourStruct TextColour, c;

    t1 = ANIM_PARAM(TIMER(GS_FILE_PICKER), UI_FADE_TIME);
    t1 = Casteljau1D(t1, EaseOut);

    TopLeft = AABB[AABB_MENU_ITEM_4][0];
    BottomRight = AABB[AABB_MENU_ITEM_4][1];
    p = POINT(BottomRight.x, ANIM(t1, TopLeft.y + 10, BottomRight.y));
    BorderTop = POINT_ADD(POINT(10, 10), TopLeft);
    BorderBottom = POINT_ADD(POINT(-10, -10), p);

    ProtonSetFG(UI_BACKGROUND_COLOUR);
    ProtonFillRect(TopLeft, p);

    ProtonSetFG(UI_BORDER_COLOUR);
    ProtonDrawRect(BorderTop, BorderBottom);

    if (TIMER(GS_FILE_PICKER) < UI_FADE_TIME)
        return;

    t1 = ANIM_PARAM(TIMER(GS_FILE_PICKER) - UI_FADE_TIME, UI_TEXT_FADE_TIME);
    t1 = Casteljau1D(t1, EaseIn);

    for (i = 0; i < FILE_PICKER_ITEMS_TOTAL; i++) {
        k = AABB_MENU_ITEM_4 + i + 1;
        if (Scroll + i >= LoadedFilesSize)
            continue;

        t2 = ANIM_PARAM(TIMER(AABB_HOVER + k), UI_TEXT_FADE_TIME);
        t2 = Casteljau1D(t2, EaseOut);
        
        c = LoadedFiles[i + Scroll].Type ? TEXT_DIR_NHOVERED_COLOUR :
                                           TEXT_FILE_NHOVERED_COLOUR;

        c = ColourAdd(ColourMult(c, 1 - t2),
                      ColourMult(TEXT_HOVERED_COLOUR, t2));
        TextColour = ColourAdd(ColourMult(UI_BACKGROUND_COLOUR, 1 - t1),
                               ColourMult(c, t1));

        ProtonSetFG(TextColour);

        p = POINT(AABB[k][0].x + 2, AABB[k][0].y + 5);

        ProtonDrawText(LoadedFiles[i + Scroll].Name, p);
    }

}

int Compare(const void *p1, const void *p2)
{
    const struct FileInfo
        f1 = *(const struct FileInfo*)p1,
        f2 = *(const struct FileInfo*)p2;

    return strcoll(f1.Name, f2.Name);
}

size_t CountFiles(DIR *Dir)
{
    struct dirent *Ent;
    size_t
        Ret = 0;

    rewinddir(Dir);
    while ((Ent = readdir(Dir)) != NULL) {
        if (Ent -> d_name[0] == '.' &&
            Ent -> d_name[1] != '.' &&
            Ent -> d_name[1])
            continue;
        Ret++;
    }

    return Ret;
}

void LoadFiles()
{
    DIR *Dir;
    IntType i;
    struct dirent *Ent;

    Dir = opendir(CurrentDir);
    /* TODO: Add error checks.                                                  */
    LoadedFilesSize = CountFiles(Dir);
    free(LoadedFiles);
    LoadedFiles = malloc(sizeof(*LoadedFiles) * LoadedFilesSize);

    /* TODO: Add error checks.                                                  */
    i = 0;
    rewinddir(Dir);
    while ((Ent = readdir(Dir)) != NULL) {
        if (Ent -> d_name[0] == '.' &&
            Ent -> d_name[1] != '.' &&
            Ent -> d_name[1])
            continue;
        strcpy(LoadedFiles[i].Name, Ent -> d_name);
        LoadedFiles[i++].Type = Ent -> d_type == DT_DIR;
    }

    qsort(LoadedFiles,
          LoadedFilesSize,
          sizeof(*LoadedFiles),
          Compare);

    closedir(Dir);
}

void ChangeCurrentPath(const char *Picked)
{
    char Tmp[PATH_MAX * 2 + 1];

    sprintf(Tmp, "%s/%s", CurrentDir, Picked);
    (void)realpath(Tmp, CurrentDir);
    LoadFiles();
}

void FilePickerSetRoot(PointStruct Point)
{
    Root = Point;
}
