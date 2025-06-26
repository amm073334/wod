#pragma once

#include <iostream>
#include <sstream>
#include <cstdint>
#include <string>
#include <vector>
#include <array>

// TODO: make format more complete, with correct integer widths etc
struct DB {
    // enums
    enum PropertyType {
        PROP_INT = 1000,
        PROP_STR = 2000,
    };

    // data
    int TYPE_ID = 0;
    int DATANAME_LOAD_TYPE = 0;
    std::string DATANAME_LOAD_NAME;

    std::string TYPENAME;
    std::string MEMO;

    std::array<int, 100> ITEMTYPE;

    struct Property {
        PropertyType type;
        std::string ITEMNAME;
        std::string VMEMO;
        std::string CHOICE_NAME;
        int CHOICE_VAL = 0;
        int DEFAULT_VAL = 0;
    };
    std::vector<Property> properties;

    std::string to_string() {
        std::ostringstream out;

        out << "TYPE_ID=" << TYPE_ID << std::endl;
        out << "DATANAME_LOAD_TYPE=" << DATANAME_LOAD_TYPE << std::endl;
        out << "DATANAME_LOAD_NAME=" << DATANAME_LOAD_NAME << std::endl;

        out << "ITEM_NUM=" << properties.size() << std::endl;
        size_t n_int_prop = 0;
        size_t n_str_prop = 0;
        for (size_t i = 0; i < properties.size(); i++) {
            if (properties.at(i).type == PROP_INT) {
                out << "DATATYPE_" << i << "="
                    << properties.at(i).type + (n_int_prop++) << std::endl;
            } else {
                out << "DATATYPE_" << i << "="
                    << properties.at(i).type + (n_str_prop++) << std::endl;
            } 
        }
        out << "NUMDATA_TYPE_NUM=" << n_int_prop << std::endl;
        out << "STRDATA_TYPE_NUM=" << n_str_prop << std::endl;

        out << "DATA_NUM=" << 0 << std::endl; // TODO
        out << "TYPENAME=" << TYPENAME << std::endl;
        out << "ITEMNAME_NUM=" << properties.size() << std::endl;
        
        for (size_t i = 0; i < properties.size(); i++)
            out << "ITEMNAME" << i << "=" << properties.at(i).ITEMNAME << std::endl;
        
        out << "MEMO=" << MEMO << std::endl;
        
        out << "ITEMTYPE_NUM=" << ITEMTYPE.size() << std::endl;
        for (size_t i = 0; i < ITEMTYPE.size(); i++)
            out << "ITEM_LOADTYPE_" << i << "=" << 0 << std::endl;
        
        out << "VMEMO_NUM=" << properties.size() << std::endl;
        for (size_t i = 0; i < properties.size(); i++)
            out << "VMEMO_NAME_" << i << "=" << properties.at(i).VMEMO << std::endl;
        
        out << "CHOICE_NUM=" << properties.size() << std::endl;
        for (size_t i = 0; i < properties.size(); i++)
            out << "CHOICE_NAME_" << i << "_NUM=" << 0 << std::endl;

        out << "CHOICE_VAL_NUM=" << properties.size() << std::endl;
        for (size_t i = 0; i < properties.size(); i++)
            out << "CHOICE_VAL_" << i << "_NUM=" << 0 << std::endl;

        out << "DEFAULT_VAL_NUM=" << properties.size() << std::endl;
        for (size_t i = 0; i < properties.size(); i++)
            out << "DEFAULT_VAL_" << i << "=" << properties.at(i).DEFAULT_VAL << std::endl;

        out << "<<--CSV_START-->>" << std::endl;
        for (Property& p : properties)
            out << "\"" << p.ITEMNAME << "\",";
        out << std::endl;
        // TODO
        out << std::endl;
        out << "<<--CSV_END-->>" << std::endl;

        return out.str();
    }
};