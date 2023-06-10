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
static int Compare(const void *p1, const void *p2);

void FilePickerUpdate(IntType Delta)
{
}

void FilePickerDraw()
{

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
