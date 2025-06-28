#pragma once

// does some simple optimization on the resulting code
#include <stack>
#include <unordered_set>
#include <string>
#include "commonevent.h"

// remove jumps that jump to the immediately following command
// and labels that are unused
void targopt_label(CommonEvent& cev) {
    std::unordered_set<std::string> jumps;
    for (size_t i = 0; i < cev.commands.size() - 1; i++) {
        Command& c = cev.commands.at(i);

        if (c.command_id == CMD_JUMP) {
            jumps.insert(c.str_fields.at(0));
            
            Command& c1 = cev.commands.at(i + 1);
            if (c1.command_id == CMD_LABEL &&
                c.str_fields.at(0) == c1.str_fields.at(0)) {
                c.command_id = 0;
                c1.command_id = 0;
            }
        }
    }

    for (size_t i = 0; i < cev.commands.size() - 1; i++) {
        Command& c = cev.commands.at(i);

        if (c.command_id == CMD_LABEL &&
            !jumps.count(c.str_fields.at(0)))
                c.command_id = 0;
    }
    cev.commands.erase(std::remove_if(cev.commands.begin(), cev.commands.end() - 1,
        [](Command& c) { return c.command_id == 0; }), cev.commands.end() - 1);
}

// look for 1-loops, potentially containing break statements,
// and remove them where it would not break control flow
void targopt_1loop(CommonEvent& cev) {
    std::stack<size_t> loop_indexes;
    std::stack<bool> can_prune;
    for (size_t i = 0; i < cev.commands.size(); i++) {
        Command& c = cev.commands.at(i);
        if (c.command_id == CMD_LOOP_COUNT) {
            loop_indexes.push(i);
            can_prune.push(c.int_fields.at(0) == 1);
        } else if (c.command_id == CMD_BREAK && !loop_indexes.empty()) {
            if (cev.commands.at(i + 1).command_id == CMD_LOOP_END) {
                c.command_id = CMD_EMPTY;
            } else {
                can_prune.top() = false;
            }
        } else if (c.command_id == CMD_LOOP_END && !loop_indexes.empty()) {
            if (can_prune.top()) {
                cev.commands.at(loop_indexes.top()).command_id = CMD_EMPTY;
                cev.commands.at(loop_indexes.top()).int_fields = {};
                c.command_id = CMD_EMPTY;

                for (size_t j = loop_indexes.top() + 1; j < i; j++) {
                    cev.commands.at(j).indent_level--;
                }
            }
            loop_indexes.pop();
            can_prune.pop();
        }
    }

    cev.commands.erase(std::remove_if(cev.commands.begin(), cev.commands.end() - 1,
        [](Command& c) { return c.command_id == 0; }), cev.commands.end() - 1);
}