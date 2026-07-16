#ifndef WOD_GAMEDATA_H_
#define WOD_GAMEDATA_H_

#include "common.h"
#include "commonevent.h"
#include "db.h"

VEC_DEF(CommonEvent);
VEC_DEF(DB);

typedef struct {
    VEC_CommonEvent cevs;
    VEC_DB cdb;
    
    int32_t entry;
} GameData;

void gd_init(GameData *gd);
void gd_write_dir(GameData *gd, StringView out);

#endif // WOD_GAMEDATA_H_