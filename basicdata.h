#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <windows.h>
#include "commonevent.h"
#include "db.h"

struct BasicData {
    std::vector<CommonEvent> cevs;
    std::vector<DB> cdbs;

    void write(std::string outdir) {
        if (!CreateDirectoryA(outdir.c_str(), NULL) && ERROR_ALREADY_EXISTS != GetLastError()) {
            std::cout << "failed" << std::endl;
            return;
        }

        if (outdir.back() != '\\' && outdir.back() != '/')
            outdir += '\\';

        outdir += "BasicData\\";
        if (!CreateDirectoryA(outdir.c_str(), NULL) && ERROR_ALREADY_EXISTS != GetLastError()) {
            std::cout << "failed" << std::endl;
            return;
        }

        std::ofstream of;
        of.open(outdir + "CommonEvent.dat.Auto.txt");
        of << "[COMMON_EVENT_TEXT_OUTPUT]" << std::endl;
        of << "COMMON_EVENT_NUM=" << std::to_string(cevs.size()) << std::endl;
        for (CommonEvent &cev : cevs) {
            of << "--------------------------\n";
            of << cev.to_string();
        }
        of.close();

        of.open(outdir + "CDataBase.Auto.txt");
        of << "[DATABASE_TEXT_OUTPUT]" << std::endl;
        of << "TYPE_NUM=" << std::to_string(cdbs.size()) << std::endl;
        for (DB &cdb : cdbs) {
            of << "----\n";
            of << cdb.to_string();
        }
        of.close();
    }
};