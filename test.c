#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "common.h"

static const char *tests[] = {
    "fail_0.wod",
    "fail_nomain.wod",
    NULL
};

static bool starts_with(const char *str, const char *substr) {
    for (size_t i = 0; substr[i] != '\0'; i++) {
        if (str[i] != substr[i]) return false;
    }

    return true;
}

int main() {
    Arena arena;
    arena_init(&arena);

    for (size_t i = 0;; i++) {
        if (!tests[i]) break;

        StringView command = 
            sv_concat(&arena, SV(".\\main.exe test\\"), to_sv(tests[i]));
        
        if (sv_is_null(command)) {
            fprintf(stderr, "Failed to allocate memory.");
            exit(1);
        }

        bool expected = starts_with(tests[i], "fail");
        bool ret = system(command.data);

        if (ret != expected)
            fprintf(stderr, "Failed: %s\n", tests[i]);
        else
            fprintf(stderr, "Passed: %s\n", tests[i]);
    }
    
    arena_free(&arena);
    return 0;
}