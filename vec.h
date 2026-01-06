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

#define VEC_PUSH(vec, item, arena) \
    do { \
        if ((vec).count >= (vec).capacity) { \
            size_t new_capacity; \
            if ((vec).capacity == 0) \
                new_capacity = VEC_MIN_CAPACITY; \
            else \
                new_capacity = (vec).capacity * 2; \
 \
            void *new_ptr = arena_alloc(arena, \
                new_capacity * sizeof((vec).at[0])); \
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

#endif // WOD_VEC_H_