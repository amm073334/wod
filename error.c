#include <stdio.h>

#include "error.h"

void error(StringView file, const char *source, Token *token, StringView message) {
    if (!token) {
        fprintf(stderr, "[Error] " SV_FMT "\n\n", SV_FMT_VAL(message));
        return;
    }

    const char *line_start;
    if (token->line == 1)
        line_start = source;
    else {
        line_start = token->text.data;
        while (line_start[-1] != '\n')
            line_start--;
    }

    const char *line_end = line_start;
    while (line_end[0] != '\n' && line_end[0] != '\0')
        line_end++;

    fprintf(stderr, "[Error] line: %zu, col: %zu, " SV_FMT "\n",
        token->line, token->col, SV_FMT_VAL(file));
    fprintf(stderr, "%6zu | %.*s\n", token->line,
        (int)(line_end - line_start), line_start);
    fprintf(stderr, "%*s", 8 + (int)token->col, "");
    for (size_t i = 0; i < token->text.len; i++) {
        fprintf(stderr, "^");
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "%*s", 8 + (int)token->col, "");
    fprintf(stderr, SV_FMT "\n\n", SV_FMT_VAL(message));
}