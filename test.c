#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "windows.h"

#define EXPECT_STR      "// EXPECT: "
#define TEST_OUTPUT     "test\\bin\\test_output"
#define TEST_TIMEOUT_MS 10000

#define C_RED "\x1b[31m"
#define C_GRN "\x1b[32m"
#define C_RESET "\x1b[m"

static bool starts_with(const char *str, const char *substr) {
    for (size_t i = 0; substr[i] != '\0'; i++) {
        if (str[i] != substr[i]) return false;
    }

    return true;
}

static void failed(const char *name, const char *message) {
    fprintf(stderr, C_RED "Failed: %s" C_RESET " (%s)\n", name, message);
}

static void passed(const char *name) {
    fprintf(stderr, C_GRN "Passed: %s\n" C_RESET, name);
}

static StringView read_from_path(const char *path, Arena *arena) {
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

static StringView read_from_handle(HANDLE handle, Arena *arena) {
    DWORD file_size = GetFileSize(handle, NULL);
    char *buf = arena_alloc(arena, file_size + 1);
    if (!buf) {
        fprintf(stderr, "Not enough memory to read file.\n");
        return SV_NULL;
    }

    DWORD n;
    if (!ReadFile(handle, buf, file_size, &n, NULL)) {
        fprintf(stderr, "Could not read file.\n");
        return SV_NULL;
    }

    assert(n == file_size);

    buf[n] = '\0';
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
        fprintf(stderr, "Failed to allocate memory.\n");
        exit(1);
    }

    // If test should fail, just check exit code.
    int ret = system(command.data);
    if (starts_with(path, "test/fail")) {
        if (ret != 2) failed(path, "Test that should fail to compile compiled successfully.");
        else passed(path);
        goto end;
    }

    // If test should compile successfully, exit code should be 0.
    if (ret != 0) {
        failed(path, "Test failed to compile.");
        goto end;
    }

    // If test should compile, and contains expected
    // output in first line, check that output matches.
    StringView f = read_from_path(path, &arena);
    if (sv_is_null(f)) goto end;

    if (!starts_with(f.data, EXPECT_STR)) {
        printf("(Note: Compiled only.) ");
        passed(path);
        goto end;
    }

    size_t newline_pos = sizeof(EXPECT_STR);
    for (; newline_pos < f.len; newline_pos++) {
        if (f.data[newline_pos] == '\n') break;
    }

    StringView expected = (StringView){
        .data = f.data + sizeof(EXPECT_STR) - 1,
        .len = newline_pos - sizeof(EXPECT_STR)
    };

    // Get a handle to the test output file.
    HANDLE file_handle;
    {
        file_handle = CreateFileA(TEST_OUTPUT,
            GENERIC_READ, FILE_SHARE_WRITE, NULL,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if (file_handle == INVALID_HANDLE_VALUE) {
            fprintf(stderr, "Failed to obtain a handle for the test output file.\n");
            goto end;
        }
    }

    PROCESS_INFORMATION pi;
    {
        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
    
        if (!CreateProcessA("test\\bin\\Game.exe",
            NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
    
            fprintf(stderr, "Failed to create process.\n");
            goto end;
        }

        DWORD event = WaitForSingleObject(pi.hProcess, TEST_TIMEOUT_MS);
        if (event == WAIT_TIMEOUT) {
            failed(path, "Timed out.");
        } else if (event != WAIT_OBJECT_0) {
            fprintf(stderr, "WaitForSingleObject failed.\n");
        }

        if (event != WAIT_OBJECT_0) {
            if (!TerminateProcess(pi.hProcess, 1)) {
                fprintf(stderr, "TerminateProcess failed.\n");
            }
            goto close_proc;
        }
    }

    StringView test_output = read_from_handle(file_handle, &arena);
    if (sv_is_null(test_output)) {
        failed(path, "No output.");
        goto close_proc;
    }

    if (sv_equals(expected, test_output)) {
        passed(path);
    } else {
        failed(path, "Unexpected output.");
        printf("Expected: " SV_FMT "; Actual: " SV_FMT "\n",
            SV_FMT_VAL(expected), SV_FMT_VAL(test_output));
    }

    close_proc:
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    end:
    arena_free(&arena);
    return 0;
}