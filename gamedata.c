#include <windows.h>
#include <string.h>

#include "gamedata.h"

static const char *dummy_pre =
    "[MAPDATA_TEXT_OUTPUT]\n"
    "MAPDATA_EXIST=1\n"
    "TILESET_ID=0\n"
    "MAP_WIDTH=10\n"
    "MAP_HEIGHT=8\n"
    "MAPLAYER=3\n"
    "[--MAP_START--]\n"
    "[MAPLAYER_1_START]\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "[MAPLAYER_1_END]\n"
    "[MAPLAYER_2_START]\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "[MAPLAYER_2_END]\n"
    "[MAPLAYER_3_START]\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "100000,100000,100000,100000,100000,100000,100000,100000,100000,100000,\n"
    "[MAPLAYER_3_END]\n"
    "[--MAP_END--]\n"
    "----------\n"
    "[EVENTDATA_TEXT_OUTPUT]\n"
    "EVENT_NUM=1\n"
    "-----\n"
    "EVENT_ID=0\n"
    "EVENT_NAME=\n"
    "EVENT_X=0\n"
    "EVENT_Y=0\n"
    "EVENT_PAGE=1\n"
    "OPTION_DATA_NUM=0\n"
    "EVENT_PAGE_NUM=1\n"
    "EVENT_PAGE=0\n"
    "EVENT_TILE_ID=-1\n"
    "GRAPHIC_FILENAME=\n"
    "EVENT_DIRECTION=2\n"
    "EVENT_PATTERN=1\n"
    "EVENT_OPACITY=255\n"
    "EVENT_BLENDTYPE=0\n"
    "EVENT_TRIGGER_TYPE=1\n"
    "EVENT_CONDITION_0_USE=33\n"
    "EVENT_CONDITION_TARGET_0=1000000\n"
    "EVENT_CONDITION_VALUE_0=0\n"
    "EVENT_CONDITION_1_USE=32\n"
    "EVENT_CONDITION_TARGET_1=1000000\n"
    "EVENT_CONDITION_VALUE_1=0\n"
    "EVENT_CONDITION_2_USE=32\n"
    "EVENT_CONDITION_TARGET_2=1000000\n"
    "EVENT_CONDITION_VALUE_2=0\n"
    "EVENT_CONDITION_3_USE=32\n"
    "EVENT_CONDITION_TARGET_3=1000000\n"
    "EVENT_CONDITION_VALUE_3=0\n"
    "ANIME_SPEED=3\n"
    "MOVE_SPEED=3\n"
    "MOVE_FREQUENCY=3\n"
    "MOVE_TYPE=0\n"
    "CHARA_OPTION_FLAG=35\n"
    "ROUTE_OPTION_FLAG=2\n"
    "ROUTE_COMMAND_NUM=0\n"
    "OPTION_VAL_NUM=3\n"
    "OPTION_VAL_0=0\n"
    "OPTION_VAL_1=0\n"
    "OPTION_VAL_2=0\n"
    "COMMAND_NUM=3\n"
    "--\n"
    "WoditorEvCOMMAND_START\n"
    "[210][2,0]<0>(";
    
static const char *dummy_post =
    ",0)()\n"
    "[121][4,0]<0>(1100000,1,0,0)()\n"
    "[0][0,0]<0>()()\n"
    "WoditorEvCOMMAND_END\n"
    "--\n"
    "[COMMAND_TEXT_START]\n"
    "[COMMAND_TEXT_END]\n";

void gd_init(GameData *gd) {
    VEC_INIT(gd->cevs);
    VEC_INIT(gd->cdb);
    gd->entry = 500000;
}

void gd_write_dir(GameData *gd, StringView out) {
    Arena arena;
    arena_init(&arena);

    char *outdir = sv_dup(&arena, out);

    if (!CreateDirectoryA(outdir, NULL) && ERROR_ALREADY_EXISTS != GetLastError()) {
        fprintf(stderr, "Failed to create directory for game data.\n");
        goto cleanup;
    }
    
    size_t base_len = strlen(outdir);
    StringView outdir_sv;
    if (outdir[base_len - 1] != '\\' && outdir[base_len - 1] != '/') {
        outdir_sv = sv_concat(&arena, to_sv(outdir), SV("\\"));
        if (!outdir_sv.data)
            goto alloc_error;
    } else {
        outdir_sv = to_sv(outdir);
    }
    
    StringView basic_data_dir =
        sv_concat(&arena, outdir_sv, SV("BasicData\\"));
    if (!basic_data_dir.data)
        goto alloc_error;

    StringView map_data_dir =
        sv_concat(&arena, outdir_sv, SV("MapData\\"));
    if (!map_data_dir.data)
        goto alloc_error;

    // Create directories.
    if (!CreateDirectoryA(basic_data_dir.data, NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS) {
        fprintf(stderr, "Failed to create BasicData directory.");
        goto cleanup;
    }
    if (!CreateDirectoryA(map_data_dir.data, NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS) {
        fprintf(stderr, "Failed to create MapData directory.");
        goto cleanup;
    }

    {
        StringView path = 
            sv_concat(&arena, basic_data_dir, SV("CommonEvent.dat.Auto.txt"));
        if (!path.data)
            goto alloc_error;

        FILE *cev_dat = fopen(path.data, "wb");
        if (!cev_dat) {
            fprintf(stderr, "Failed to open CommonEvent.dat.Auto.txt.");
            goto cleanup;
        }

        fprintf(cev_dat, "[COMMON_EVENT_TEXT_OUTPUT]\n");
        fprintf(cev_dat, "COMMON_EVENT_NUM=%zu\n", gd->cevs.count);
        for (size_t i = 0; i < gd->cevs.count; i++) {
            fprintf(cev_dat, "--------------------------\n");
            cev_write_txt(&gd->cevs.at[i], cev_dat);
        }
        fclose(cev_dat);
    }

    {
        StringView path = 
            sv_concat(&arena, basic_data_dir, SV("CDataBase.Auto.txt"));
        if (!path.data)
            goto alloc_error;

        FILE *cdb_dat = fopen(path.data, "wb");
        if (!cdb_dat) {
            fprintf(stderr, "Failed to open CDataBase.Auto.txt.");
            goto cleanup;
        }

        fprintf(cdb_dat, "[DATABASE_TEXT_OUTPUT]\n");
        fprintf(cdb_dat, "TYPE_NUM=%zu\n", gd->cdb.count);
        for (size_t i = 0; i < gd->cdb.count; i++) {
            fprintf(cdb_dat, "----\n");
            db_write_txt(&gd->cdb.at[i], cdb_dat);
        }
        fclose(cdb_dat);
    }

    {
        StringView path = 
            sv_concat(&arena, map_data_dir, SV("SampleMap.mps.Auto.txt"));
        if (!path.data)
            goto alloc_error;

        FILE *sample_map = fopen(path.data, "wb");
        if (!sample_map) {
            fprintf(stderr, "Failed to open SampleMap.mps.Auto.txt.");
            return;
        }
        
        fprintf(sample_map, "%s%d%s", dummy_pre, gd->entry, dummy_post);
        fclose(sample_map);
    }

    goto cleanup;

    alloc_error:
    fprintf(stderr, "Failed to allocate.");

    cleanup:
    arena_free(&arena);
}

bool gd_apply(Arena *arena, StringView editor_path, StringView txt_path) {
    StringView command = sv_concat(arena, sv_concat(arena, sv_concat(arena,
        editor_path, SV(" -txtinput -txt_folder ")), txt_path), SV(" -target ALL"));

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    bool success = CreateProcessA(NULL, command.data,
        NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return success;
}
