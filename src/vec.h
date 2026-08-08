#ifndef WOD_VEC_H_
#define WOD_VEC_H_

#include <string.h>
#include <stdlib.h>

#include "memory.h"

#define VEC_MIN_CAPACITY 8

#define VEC_DEF(T) \
    typedef struct { \
        size_t capacity; \
        size_t count; \
        T *at; \
    } VEC_##T

#define VEC_PTR_DEF(T) \
    typedef struct { \
        size_t capacity; \
        size_t count; \
        T **at; \
    } VEC_PTR_##T

#define VEC_INIT(vec) \
    do { \
        (vec).capacity = 0; \
        (vec).count = 0; \
        (vec).at = NULL; \
    } while (0)

#define VEC_EMPTY { .capacity = 0, .count = 0, .at = NULL }

#define VEC_PUSH(vec, item, arena) \
    do { \
        if ((vec).count >= (vec).capacity) { \
            if ((vec).capacity == 0) \
                (vec).capacity = VEC_MIN_CAPACITY; \
            else \
                (vec).capacity *= 2; \
 \
            void *new_ptr = arena_alloc(arena, \
                (vec).capacity * sizeof((vec).at[0])); \
            if (!new_ptr) exit(1); \
            if ((vec).at) \
                memcpy(new_ptr, (vec).at, \
                    (vec).count * sizeof((vec).at[0])); \
            (vec).at = new_ptr; \
        } \
 \
        (vec).at[(vec).count++] = (item); \
    } while (0)

#define VEC_POP(vec) ((vec).count--)

#define VEC_REMOVE(vec, pos) \
    do { \
        for (size_t _i = (pos) + 1; _i < (vec).count; _i++) { \
            (vec).at[_i - 1] = (vec).at[_i]; \
        } \
        (vec).count--; \
    } while (0)

#endif // WOD_VEC_H_