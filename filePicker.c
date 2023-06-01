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

#define FILE_PICKER_OFFSET_X (100)
#define FILE_PICKER_OFFSET_Y (100)
#define FILE_PICKER_WIDTH (WIN_WIDTH - FILE_PICKER_OFFSET_X * 2)
#define FILE_PICKER_HEIGHT (WIN_HEIGHT - FILE_PICKER_OFFSET_Y * 2)
#define FILE_PICKER_ITEM_OFFSET_X (10)
#define FILE_PICKER_ITEM_OFFSET_Y (40)
#define FILE_PICKER_ITEM_WIDTH (900)
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
static IntType
    Scroll = 0;

static void LoadFiles();
static void ChangeCurrentPath(const char *Picked);
static size_t CountFiles(DIR *Dir);
int Compare(const void *p1, const void *p2)
{
    const struct FileInfo
        f1 = *(const struct FileInfo*)p1,
        f2 = *(const struct FileInfo*)p2;

    return strcoll(f1.Name, f2.Name);
}

void FilePickerUpdate(IntType Delta)
{
    IntType i;

    IntType
        TickDelta = Tick - GameStateTicks[GAME_STATE_FILE_PICKER];

    if (!TickDelta) {
        GameState[GAME_STATE_CURVES] = FALSE;
        CurrentDir[0] = '.';
        CurrentDir[1] = '\0';
        Scroll = 0;
        LoadFiles();
    }

    RendererPushTransform(POINT(FILE_PICKER_OFFSET_X, FILE_PICKER_OFFSET_Y),
                          1.f); 

    AABB_SET(AABB[0], POINT_ZERO, POINT(FILE_PICKER_WIDTH, FILE_PICKER_HEIGHT));
    RendererPushTransform(POINT(50, 50), 1.f); 
    AABB_SET(AABB[1], POINT_ZERO, POINT(FILE_PICKER_WIDTH - 100, FILE_PICKER_HEIGHT - 100));
    RendererPushTransform(POINT(FILE_PICKER_ITEM_OFFSET_X, 0), 1.f); 

    for (i = 0; i < FILE_PICKER_ITEMS_TOTAL; i++) {
        
        RendererPushTransform(POINT(0, FILE_PICKER_ITEM_OFFSET_Y),
                              1.f); 

        AABB_SET(AABB[2 + i], POINT_ZERO,
                              POINT(FILE_PICKER_ITEM_WIDTH, FILE_PICKER_ITEM_HEIGHT));
    }

    for (i = 0; i < FILE_PICKER_ITEMS_TOTAL; i++)
        RendererPopTransform();

    RendererPopTransform();
    RendererPopTransform();
    RendererPopTransform();

    if (MouseKeys[M2] && !LastMouseKeys[M2]) {
        GameState[GAME_STATE_FILE_PICKER] = FALSE;
        GameState[GAME_STATE_CURVES] = TRUE;
    }
    Scroll -= (MouseWheel < 0 ? -1 : 1) * (MouseWheel != 0);
    Scroll = MAX(MIN(Scroll, LoadedFilesSize - 4), 0);

    if (MouseKeys[M1] && !LastMouseKeys[M1]) {
        for (i = 0; i < FILE_PICKER_ITEMS_TOTAL; i++) {
            if (Scroll + i >= LoadedFilesSize)
                continue;

            if (AABBClickTicks[2 + i] != INACTIVE && LoadedFiles[i + Scroll].Type)
                ChangeCurrentPath(LoadedFiles[i + Scroll].Name);
        }
    }
}

void FilePickerDraw()
{
    IntType i, TickDelta;
    RealType t;
    PointStruct p;

    RendererSetFGColour(0, 255, 255);
    RendererFillRectangle(AABB[0][0], AABB[0][1]);

    RendererSetFGColour(0, 0, 255);
    RendererFillRectangle(AABB[1][0], AABB[1][1]);
    for (i = 0; i < FILE_PICKER_ITEMS_TOTAL; i++) {
        if (Scroll + i >= LoadedFilesSize)
            continue;

        if (AABBHighlightTicks[2 + i] != INACTIVE) {
            TickDelta = Tick - AABBHighlightTicks[2 + i];
            t = TickDelta / 30.f;
            t = MIN(t, 1.f);

            t = Casteljau1D(t, EaseOut);
            RendererSetFGColour(255, 255, 0);
            p = POINT(AABB[2 + i][1].x * t + AABB[2 + i][0].x * (1 - t),
                    AABB[2 + i][1].y);
            RendererFillRectangle(AABB[2 + i][0], p);
        }

        RendererSetFGColour(255, 255, 0);
        RendererDrawRectangle(AABB[2 + i][0], AABB[2 + i][1]);
        if (LoadedFiles[i + Scroll].Type)
            RendererSetFGColour(255, 255, 255);
        else
            RendererSetFGColour(0, 0, 0);

        p = POINT(AABB[2 + i][0].x + 2, AABB[2 + i][0].y + 5);

        RendererDrawText(LoadedFiles[i + Scroll].Name, p);
    }

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
