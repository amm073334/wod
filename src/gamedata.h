#ifndef WOD_GAMEDATA_H_
#define WOD_GAMEDATA_H_

#include "common.h"
#include "commonevent.h"
#include "db.h"

VEC_DEF(CommonEvent);

typedef struct {
    VEC_CommonEvent cevs;
    VEC_DBType udb;
    VEC_DBType cdb;
    VEC_DBType sdb;
    
    int32_t entry;
} GameData;

void gd_init(GameData *gd);
void gd_write_dir(GameData *gd, StringView out);

// `txt_path` should be a relative path from the `editor_path`.
bool gd_apply(Arena *arena, StringView editor_path, StringView txt_path);

#endif // WOD_GAMEDATA_H_