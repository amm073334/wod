#ifndef WOD_LOCATION_H_
#define WOD_LOCATION_H_

#include "common.h"
#include "source.h"

typedef struct Location {
    Source source;
    size_t line;
    size_t column;
    size_t length;
} Location;

#endif // WOD_LOCATION_H_