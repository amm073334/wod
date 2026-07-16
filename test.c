#include <stdio.h>
#include <stdlib.h>

#include "common.h"

#define C_RED "\x1b[31m"
#define C_GRN "\x1b[32m"
#define C_RESET "\x1b[m"
#define EXPECT_STR "// EXPECT: "

static bool starts_with(const char *str, const char *substr) {
    for (size_t i = 0; substr[i] != '\0'; i++) {
        if (str[i] != substr[i]) return false;
    }

    return true;
}

static void failed(const char *name) {
    fprintf(stderr, C_RED "Failed: %s\n" C_RESET, name);
}

static void passed(const char *name) {
    fprintf(stderr, C_GRN "Passed: %s\n" C_RESET, name);
}

static StringView read_file(const char *path, Arena *arena) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Could not open file '%s'.", path);
        return SV_NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char *buf = arena_alloc(arena, file_size + 1);
    if (!buf) {
        fprintf(stderr, "Not enough memory to read '%s'.", path);
        return SV_NULL;
    }

    size_t n = fread(buf, sizeof(char), file_size, file);
    if (n < file_size) {
        fprintf(stderr, "Could not read '%s'.", path);
        return SV_NULL;
    }

    buf[n] = '\0';
    fclose(file);

    return (StringView){ .data = buf, .len = n };
}

int main(int argc, char **argv) {
    if (argc != 2) exit(1);

    const char *path = argv[1];
    
    Arena arena;
    arena_init(&arena);

    StringView command = 
        sv_concat(&arena, SV(".\\wodc.exe "), to_sv(path));
    
    command = sv_concat(&arena, command, to_sv(" 2> nul"));

    if (sv_is_null(command)) {
        fprintf(stderr, "Failed to allocate memory.");
        exit(1);
    }

    // If test should fail, just check exit code.
    int ret = system(command.data);
    if (starts_with(path, "test/fail")) {
        if (ret != 2) failed(path);
        else passed(path);
        goto end;
    }

    // If test should compile successfully, exit code should be 0.
    if (ret != 0) {
        failed(path);
        goto end;
    }

    // If test should compile, and contains expected
    // output in first line, check that output matches.
    StringView f = read_file(path, &arena);
    if (sv_is_null(f)) goto end;

    if (!starts_with(f.data, EXPECT_STR)) {
        passed(path);
        goto end;
    }

    size_t newline_pos = sizeof(EXPECT_STR);
    for (; newline_pos < f.len; newline_pos++) {
        if (f.data[newline_pos] == '\n') break;
    }

    StringView expected = (StringView){
        .data = f.data + sizeof(EXPECT_STR),
        .len = newline_pos - sizeof(EXPECT_STR)
    };

    // TODO: apply changes, run Game.exe, compare

    end:
    arena_free(&arena);
    return 0;
}