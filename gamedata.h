#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <windows.h>
#include "common.h"
#include "commonevent.h"
#include "db.h"

struct GameData {
    std::vector<CommonEvent> cevs;
    std::vector<DB> cdbs;
    int32_t entry = CEV_THRESHOLD;

    void write(std::string outdir) {
        if (!CreateDirectoryA(outdir.c_str(), NULL) && ERROR_ALREADY_EXISTS != GetLastError()) {
            std::cout << "failed" << std::endl;
            return;
        }

        if (outdir.back() != '\\' && outdir.back() != '/')
            outdir += '\\';

        std::string bd_dir = outdir + "BasicData\\";
        if (!CreateDirectoryA(bd_dir.c_str(), NULL) && ERROR_ALREADY_EXISTS != GetLastError()) {
            std::cout << "failed" << std::endl;
            return;
        }

        std::string map_dir = outdir + "MapData\\";
        if (!CreateDirectoryA(map_dir.c_str(), NULL) && ERROR_ALREADY_EXISTS != GetLastError()) {
            std::cout << "failed" << std::endl;
            return;
        }

        std::ofstream of;
        of.open(bd_dir + "CommonEvent.dat.Auto.txt");
        of << "[COMMON_EVENT_TEXT_OUTPUT]" << std::endl;
        of << "COMMON_EVENT_NUM=" << std::to_string(cevs.size()) << std::endl;
        for (CommonEvent &cev : cevs) {
            of << "--------------------------\n";
            of << cev.to_string();
        }
        of.close();

        of.open(bd_dir + "CDataBase.Auto.txt");
        of << "[DATABASE_TEXT_OUTPUT]" << std::endl;
        of << "TYPE_NUM=" << std::to_string(cdbs.size()) << std::endl;
        for (DB &cdb : cdbs) {
            of << "----\n";
            of << cdb.to_string();
        }
        of.close();

        of.open(map_dir + "SampleMap.mps.Auto.txt");
        of << dummy_pre << entry << dummy_post;
        of.close();
    }

    std::string dummy_pre =
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
    std::string dummy_post =
        ",0)()\n"
        "[121][4,0]<0>(1100000,1,0,0)()\n"
        "[0][0,0]<0>()()\n"
        "WoditorEvCOMMAND_END\n"
        "--\n"
        "[COMMAND_TEXT_START]\n"
        "[COMMAND_TEXT_END]\n";
};