#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "memory.h"

#define ARENA_MIN_ALLOC 16384

struct ArenaBlock {
    ArenaBlock *next;
    size_t capacity;
    size_t size;

    // Flexible array.
    // MSVC seems to complain if this is written as
    // `uint8_t data[]` for some reason, so it has one element.
    uint8_t data[1];
};

void arena_init(Arena *arena) {
    arena->head = NULL;
}

static ArenaBlock *alloc_block(size_t capacity) {
    ArenaBlock *b = malloc(sizeof(ArenaBlock) + capacity);
    if (!b) return NULL;
    
    b->next = NULL;
    b->capacity = capacity;
    b->size = 0;
    return b;
}

void *arena_alloc(Arena *arena, size_t size) {
    if (size == 0)
        return NULL;

    if (!arena->head) {
        arena->head = alloc_block(
            size < ARENA_MIN_ALLOC ? ARENA_MIN_ALLOC : size);
        if (!arena->head) return NULL;
    }

    ArenaBlock *head = arena->head;
    
    // https://en.wikipedia.org/wiki/Data_structure_alignment#Computing_padding
    const size_t align = sizeof(void *);
    size_t padding = (-(intptr_t)(head->data + head->size) & (align - 1));
    if (head->size + padding + size > head->capacity) {
        ArenaBlock *b = alloc_block(
            size < ARENA_MIN_ALLOC ? ARENA_MIN_ALLOC : size);
        if (!b) return NULL;

        b->next = head;
        return b;
    }

    size_t old_size = head->size;
    head->size += padding + size;
    return head->data + old_size + padding;
}

void *arena_alloc_assert(Arena *arena, size_t size) {
    void *out = arena_alloc(arena, size);
    if (!out) {
        fprintf(stderr, "Fatal error: Out of memory.\n");
        exit(1);
    }
    return out;
}

void arena_free(Arena *arena) {
    ArenaBlock *b = arena->head;
    while (b) {
        ArenaBlock *next = b->next;
        free(b);
        b = next;
    }
    arena_init(arena);
}