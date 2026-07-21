#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "windows.h"

#define C_RED "\x1b[31m"
#define C_GRN "\x1b[32m"
#define C_RESET "\x1b[m"
#define EXPECT_STR "// EXPECT: "
#define TEST_OUTPUT "test\\bin\\test_output"

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
        fprintf(stderr, "Could not open file '%s'.\n", path);
        return SV_NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char *buf = arena_alloc(arena, file_size + 1);
    if (!buf) {
        fprintf(stderr, "Not enough memory to read '%s'.\n", path);
        return SV_NULL;
    }

    size_t n = fread(buf, sizeof(char), file_size, file);
    if (n < file_size) {
        fprintf(stderr, "Could not read '%s'.\n", path);
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
    
    command = sv_concat(&arena, command, to_sv(" 2> nul 1> nul"));

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
        printf("(compile only) ");
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

    {
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
    
        if (!CreateProcessA("test\\bin\\Game.exe",
            NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
    
            fprintf(stderr, "Failed to create process.");
            goto end;
        }
    
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    StringView test_output = read_file(TEST_OUTPUT, &arena);
    if (sv_is_null(test_output)) {
        printf("(no output) ");
        failed(path);
        goto end;
    }

    if (!sv_equals(expected, test_output)) {
        failed(path);
        printf("Expected: " SV_FMT "; Actual: " SV_FMT "\n",
            SV_FMT_VAL(expected), SV_FMT_VAL(test_output));
        goto end;
    }

    if (!DeleteFile(TEST_OUTPUT)) {
        printf("(delete failed) ");
        failed(path);
        goto end;
    }

    passed(path);

    end:
    arena_free(&arena);
    return 0;
}