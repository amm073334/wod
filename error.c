#include <stdio.h>

#include "error.h"

static char *get_nth_line(Source source, size_t line) {
    size_t cur_line = 1;
    for (size_t i = 0; i < source.text.len; i++) {
        if (line == cur_line)
            return &source.text.data[i];

        switch (source.text.data[i]) {
        case '\0': return NULL;
        case '\n': cur_line++; break;
        }
    }
    return NULL;
}

void error(Location loc, size_t len, StringView message) {
    const char *line_start = get_nth_line(loc.source, loc.line);

    const char *line_end = line_start;
    while (line_end[0] != '\n' && line_end[0] != '\0')
        line_end++;

    fprintf(stderr, "[Error] line: %zu, col: %zu, " SV_FMT "\n",
        loc.line, loc.column, SV_FMT_VAL(loc.source.path));
    fprintf(stderr, "%6zu | %.*s\n", loc.line,
        (int)(line_end - line_start), line_start);
    fprintf(stderr, "%*s", 8 + (int)loc.column, "");
    for (size_t i = 0; i < len; i++) {
        fprintf(stderr, "^");
    }
    fprintf(stderr, "\n");

    fprintf(stderr, "%*s", 8 + (int)loc.column, "");
    fprintf(stderr, SV_FMT "\n\n", SV_FMT_VAL(message));
}