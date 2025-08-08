#include "ecs/utils.h"

// Arenas
void arenaInit(Arena *arena, size_t bytesize) {
  arena->head = malloc(bytesize);
  arena->tail = arena->head;
  arena->mem_left = bytesize;
}

void *arenaAlloc(Arena *arena, size_t bytesize) {
  assert((arena->mem_left -= bytesize) >= 0 && "No memory left in arena.");
  void *ptr = arena->head;
  arena->head += bytesize;
  return ptr;
}

void arenaFree(Arena *arena) {
  free(arena->tail);
  arena->mem_left = 0;
}

// Bitmasks
void bitmaskInit(Arena *arena, Bitmask *mask, u64 bitsize) {
  mask->size = (bitsize + 63) / 64;
  mask->bytesize = mask->size * sizeof(u64);
  mask->bitsize = bitsize;

  mask->bits = (u64*)arenaAlloc(arena, mask->bytesize);
  memset(mask->bits, 0, mask->bytesize);
}

void bitmaskInitCpy(Arena *arena, Bitmask *mask, Bitmask *copying) {
  bitmaskInit(arena, mask, copying->bitsize);
  memcpy(mask->bits, copying->bits, mask->bytesize);
}

bool bitmaskContains(Bitmask *mask, Bitmask *element) {
  for (u32 i = 0; i < mask->size; i++) {
    if ((mask->bits[i] & element->bits[i]) != element->bits[i]) {
      return false;
    }
  }
  return true;
}

void bitmaskPrint(Bitmask *mask) {
  printf("Bitmask<");
  for (u32 i = 0; i < mask->size; i++) {
    u64 word = mask->bits[i];

    for (u8 j = 0; j < 64; j++) {
      printf("%li", (word >> j) & 1);
    }
  }
  printf(">\n");
}

u32 bitmaskLowestFlag(Bitmask *mask) {
  for (u8 i = 0; i < mask->size; i++) {
    u64 word = mask->bits[i];

    for (u8 j = 0; j < 64; j++) {
      if ((word >> j) & 1) {
        return i * 64 + j;
      }
    }
  }
  return -1;
}

u32 bitmaskHighestFlag(Bitmask *mask) {
  for (i8 i = mask->size - 1; i >= 0; i--) {
    u64 word = mask->bits[i];

    for (i8 j = 63; j >= 0; j--) {
      if ((word >> j) & 1) {
        return i * 64 + j;
      }
    }
  }
  return -1;
}

// Brian Kernighan's algorithm :)
u32 bitmaskFlagCount(Bitmask *mask) {
  u32 count = 0;
  u64 n;
  for (u8 i = 0; i < mask->size; i++) {
    n = mask->bits[i];
    while (n) {
      n &= (n - 1);
      count++;
    }
  }
  return count;
}
