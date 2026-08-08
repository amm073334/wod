#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "windows.h"

#define EXPECT_STR          "// EXPECT: "
#define BIN_DIR             "test\\bin\\"
#define TEST_OUTPUT_FILE    "test_output"
#define TEST_TIMEOUT_MS     10000

#define C_RED "\x1b[31m"
#define C_GRN "\x1b[32m"
#define C_RESET "\x1b[m"

char *base_dir;

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

void test_one(const char *input_file) {
    Arena arena;
    arena_init(&arena);

    DWORD path_len = GetFullPathNameA(input_file, 0, NULL, NULL);
    char *path = arena_alloc(&arena, path_len);
    char *file_name = arena_alloc(&arena, path_len);
    if (!path || !file_name) {
        fprintf(stderr, "Failed to allocate memory.\n");
        exit(1);
    }
    GetFullPathNameA(input_file, path_len, path, &file_name);

    StringView command = sv_concat(&arena, sv_concat(&arena,
        to_sv(base_dir), SV("\\wodc.exe ")), to_sv(path));
    
    command = sv_concat(&arena, command, to_sv(" 2> nul 1> nul"));

    if (sv_is_null(command)) {
        fprintf(stderr, "Failed to allocate memory.\n");
        exit(1);
    }

    // If file starts with an underscore, ignore it.
    // This is useful for any libraries shared across tests.
    if (starts_with(file_name, "_")) {
        goto end;
    }

    int ret = system(command.data);
    if (ret == 1) {
        failed(file_name, "Test crashed the compiler.");
        goto end;
    }

    // If test should fail, just check exit code.
    if (starts_with(file_name, "fail")) {
        if (ret != 2) failed(file_name, "Test that should fail to compile compiled successfully.");
        else passed(file_name);
        goto end;
    }

    // If test should compile successfully, exit code should be 0.
    if (ret != 0) {
        failed(file_name, "Test failed to compile.");
        goto end;
    }

    // If test should compile, and contains expected
    // output in first line, check that output matches.
    StringView f = read_from_path(path, &arena);
    if (sv_is_null(f)) goto end;

    if (!starts_with(f.data, EXPECT_STR)) {
        printf("(Note: Compiled only.) ");
        passed(file_name);
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
        StringView test_output = sv_concat(&arena,
            to_sv(base_dir), SV("\\" BIN_DIR TEST_OUTPUT_FILE));

        file_handle = CreateFileA(test_output.data,
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
    
        StringView game = sv_concat(&arena, to_sv(base_dir), SV("\\" BIN_DIR "Game.exe"));
        if (!CreateProcessA(game.data,
            NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
    
            fprintf(stderr, "Failed to create process.\n");
            goto end;
        }

        DWORD event = WaitForSingleObject(pi.hProcess, TEST_TIMEOUT_MS);
        if (event == WAIT_TIMEOUT) {
            failed(file_name, "Timed out.");
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
        failed(file_name, "No output.");
        goto close_proc;
    }

    if (!sv_equals(expected, test_output)) {
        failed(file_name, "Unexpected output.");
        printf("Expected: " SV_FMT "; Actual: " SV_FMT "\n",
            SV_FMT_VAL(expected), SV_FMT_VAL(test_output));
        goto close_proc;
    }
    
    passed(file_name);

    close_proc:
    CloseHandle(file_handle);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    end:
    arena_free(&arena);
}

bool ends_with_wod(const char *s) {
    size_t len = strlen(s);
    if (len < 4) return false;
    if (strcmp(s + len - 3, "wod") != 0) return false;
    return true;
}

void test_dir(Arena *arena, const char *rel_path) {
    WIN32_FIND_DATAA ffd;

    DWORD full_len = GetFullPathNameA(rel_path, 0, NULL, NULL);
    char *abs_path = arena_alloc_assert(arena, full_len);
    GetFullPathNameA(rel_path, full_len, abs_path, NULL);
    SetCurrentDirectoryA(abs_path);

    StringView search = sv_concat(arena, to_sv(abs_path), SV("\\*"));
    HANDLE handle = FindFirstFileA(search.data, &ffd);
    if (handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error occurred while traversing '%s'.\n", rel_path);
        return;
    }

    do {
        if (strcmp(ffd.cFileName, ".") == 0
            || strcmp(ffd.cFileName, "..") == 0)
            continue;

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            test_dir(arena, ffd.cFileName);
            SetCurrentDirectoryA(abs_path);
        } else if (ends_with_wod(ffd.cFileName)) {
            test_one(ffd.cFileName);
        }
    } while (FindNextFileA(handle, &ffd) != 0);

    FindClose(handle);
}

int main(int argc, char **argv) {
    Arena arena;
    arena_init(&arena);

    DWORD len = GetCurrentDirectoryA(0, NULL);
    base_dir = arena_alloc_assert(&arena, len);
    GetCurrentDirectoryA(len, base_dir);

    if (argc == 2) {
        test_one(argv[1]);
        return 0;
    }
    if (argc != 3) exit(1);
    if (strcmp(argv[1], "-f") != 0) exit(1);

    test_dir(&arena, argv[2]);

    return 0;
}