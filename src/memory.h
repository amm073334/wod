#ifndef WOD_MEMORY_H_
#define WOD_MEMORY_H_

typedef struct ArenaBlock ArenaBlock;

typedef struct {
    ArenaBlock *head;
} Arena;

void arena_init(Arena *arena);
void *arena_alloc(Arena *arena, size_t size);
void *arena_alloc_assert(Arena *arena, size_t size);
void arena_free(Arena *arena);

#endif // WOD_MEMORY_H_