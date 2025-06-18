#include <iostream>
#include <sstream>
#include <cstdint>
#include <string>
#include <vector>
#include "command.h"

struct CommonEvent {
    // enums
    enum CommandColor : int32_t {
        COLOR_BLACK,
        COLOR_RED,
        COLOR_BLUE,
        COLOR_GREEN,
        COLOR_PURPLE,
        COLOR_YELLOW,
        COLOR_GRAY
    };

    enum TriggerType : int8_t {
        TR_CALLED_ONLY,
        TR_ON_CONDITION,
        TR_PARALLEL_ON_CONDITION,
        TR_PARALLEL_ALWAYS
    };

    enum TriggerComp : int8_t {
        TC_GT   = 0x00,
        TC_GTE  = 0x10,
        TC_EQ   = 0x20,
        TC_LTE  = 0x30,
        TC_LT   = 0x40,
        TC_NEQ  = 0x50,
        TC_AND  = 0x60
    };

    enum LoadType : int8_t {
        LT_NONE,
        LT_DB,
        LT_ENUM
    };

    // common event data: names are exactly as in the official text output,
    // with numbers swapped out for lowercase i or j
    int32_t COMMON_ID = 0;
    std::string COMMON_NAME;
    CommandColor COMMAND_COLOR = COLOR_BLACK;

    int8_t TRIGGER = TR_CALLED_ONLY | TC_EQ;
    int32_t TRIGGER_TARGET = 2000000;
    int32_t TRIGGER_VAL = 0;

    std::string MEMO;

    int8_t VALINPUT_NUM = 0;
    int8_t STRINPUT_NUM = 0;

    static const int32_t VALINPUT_NAME_NUM = 11;
    std::string VALINPUT_NAME_i[VALINPUT_NAME_NUM] = {};
    
    static const int32_t LOADTYPE_NUM = 10;
    LoadType LOADTYPEi[LOADTYPE_NUM] = {};
    
    static const int32_t CHOICE_STR_NUM = 10;
    int32_t CHOICE_STR_i_NUM[CHOICE_STR_NUM] = {};
    std::vector<std::string> CHOICE_STR_i_j;
    static const int32_t CHOICE_VAL_NUM = 10;
    int32_t CHOICE_VAL_i_NUM[CHOICE_VAL_NUM] = {};
    std::vector<int32_t> CHOICE_VAL_i_j;
    
    static const int32_t DEFAULT_VAL_NUM = 5;
    int32_t DEFAULT_VAL_i[DEFAULT_VAL_NUM];

    std::string RETURN_VAL_NAME;
    int32_t RETURN_VAL_TARGET = -1;

    std::string CSELF_NAME_i[100];

    int32_t COMMAND_NUM = 0;

    // class data: things that are not officially named in real editor output,
    // or auxiliary things to make the class easier to use
    int8_t current_indent = 0;
    std::vector<Command> commands;

    Command* add_cmd(int32_t command_id) {
        commands.push_back(Command{command_id, current_indent, {}, {}});
        COMMAND_NUM++;
        return &commands.back();
    }
    Command* add_cmd(int32_t command_id, std::vector<int32_t> int_fields, std::vector<std::string> str_fields) {
        commands.push_back(Command{command_id, current_indent, int_fields, str_fields});
        COMMAND_NUM++;
        return &commands.back();
    }

    void indent() { current_indent++; }
    void outdent() { current_indent--; }

    std::string to_string() {
        std::ostringstream out;

        // macros
        // the "8" versions are a compromise because streams try to write 8-bit values as chars,
        // so if a value is 8 bit we have to cast to a bigger number to avoid that

        // write identifier equal to itself
        #define WOD_CEV_WRITE(ID) do {                                              \
            out << #ID "=" << ID << "\n";                                           \
        } while (0)
        #define WOD_CEV_WRITE8(ID) do {                                             \
            out << #ID "=" << static_cast<int>(ID) << "\n";                         \
        } while (0)

        // loop that writes identifier equal to each value in identifier's array
        #define WOD_CEV_WRITE_I(ID, I) do {                                         \
            for (size_t i = 0; i < I; i++) {                                        \
                out << #ID << i << "=" << ID##i[i] << "\n";                         \
            }                                                                       \
        } while (0)
        #define WOD_CEV_WRITE_I8(ID, I) do {                                        \
            for (size_t i = 0; i < I; i++) {                                        \
                out << #ID << i << "=" << static_cast<int>(ID##i[i]) << "\n";       \
            }                                                                       \
        } while (0)

        // nested loop to write an array of lengths, as well as the contents of the arrays with those lengths
        #define WOD_CEV_WRITE_IJ(ID, I) do {                                        \
            for (size_t i = 0; i < I; i++) {                                        \
                out << #ID << i << "_NUM=" << ID##i_NUM[i] << "\n";                 \
                for (size_t j = 0; j < ID##i_NUM[i]; j++) {                         \
                    out << #ID << i << "_" << j << "=" << ID##i_j[j] << "\n";       \
                }                                                                   \
            }                                                                       \
        } while (0) 

        WOD_CEV_WRITE(COMMON_ID);
        WOD_CEV_WRITE(COMMON_NAME);
        WOD_CEV_WRITE(COMMAND_COLOR);
        WOD_CEV_WRITE8(TRIGGER);
        WOD_CEV_WRITE(TRIGGER_TARGET);
        WOD_CEV_WRITE(TRIGGER_VAL);
        WOD_CEV_WRITE(MEMO);
        WOD_CEV_WRITE8(VALINPUT_NUM);
        WOD_CEV_WRITE8(STRINPUT_NUM);
        WOD_CEV_WRITE(VALINPUT_NAME_NUM);
        WOD_CEV_WRITE_I(VALINPUT_NAME_, VALINPUT_NAME_NUM);
        WOD_CEV_WRITE(LOADTYPE_NUM);
        WOD_CEV_WRITE_I8(LOADTYPE, LOADTYPE_NUM);
        WOD_CEV_WRITE(CHOICE_STR_NUM);
        WOD_CEV_WRITE_IJ(CHOICE_STR_, CHOICE_STR_NUM);
        WOD_CEV_WRITE(CHOICE_VAL_NUM);
        WOD_CEV_WRITE_IJ(CHOICE_VAL_, CHOICE_VAL_NUM);
        WOD_CEV_WRITE(DEFAULT_VAL_NUM);
        WOD_CEV_WRITE_I(DEFAULT_VAL_, DEFAULT_VAL_NUM);
        WOD_CEV_WRITE(RETURN_VAL_NAME);
        WOD_CEV_WRITE(RETURN_VAL_TARGET);
        WOD_CEV_WRITE_I(CSELF_NAME_, 100);
        WOD_CEV_WRITE(COMMAND_NUM);
        
        #undef WOD_CEV_WRITE
        #undef WOD_CEV_WRITE8
        #undef WOD_CEV_WRITE_I
        #undef WOD_CEV_WRITE_I8
        #undef WOD_CEV_WRITE_IJ

        out << "--\n";
        out << "WoditorEvCOMMAND_START\n";

        for (Command &cmd : commands) {
            out << "[" << std::to_string(cmd.command_id) << "][" 
                << std::to_string(cmd.int_fields.size()) << "," 
                << std::to_string(cmd.str_fields.size()) << "]<" 
                << std::to_string(cmd.indent_level) << ">(";
            for (size_t i = 0; i < cmd.int_fields.size(); i++) {
                out << cmd.int_fields.at(i);
                if (i < cmd.int_fields.size() - 1) out << ",";
            }
            out << ")(";
            for (size_t i = 0; i < cmd.str_fields.size(); i++) {
                out << "\"" << cmd.str_fields.at(i) << "\"";
                if (i < cmd.str_fields.size() - 1) out << ",";
            }
            out << ")\n";
        }
        out << "WoditorEvCOMMAND_END\n";
        out << "--\n";

        // skip text generation
        out << "[COMMAND_TEXT_START]\n";
        out << "[COMMAND_TEXT_END]\n";

        return out.str();
    }
};