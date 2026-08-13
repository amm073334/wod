#ifndef WOD_COMMON_H_
#define WOD_COMMON_H_

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#include "sv.h"
#include "vec.h"

#define ARRLEN(arr) (sizeof(arr) / sizeof(arr[0]))

#define UNREACHABLE assert(false)
#define UNIMPLEMENTED assert(false)

VEC_DEF(int32_t);
VEC_DEF(StringView);

typedef enum {
    DB_UDB,
    DB_CDB
} DBKind;

#endif // WOD_COMMON_H_