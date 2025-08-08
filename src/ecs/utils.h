#ifndef ECS_UTILS_H
#define ECS_UTILS_H
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define kB * 1000
#define MB * 1000000
#define GB * 1000000000

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

// Arenas
typedef struct {
  void *head, *tail;
  i64 mem_left;
} Arena;

void arenaInit(Arena *arena, size_t bytesize);
void *arenaAlloc(Arena *arena, size_t bytesize);
void arenaFree(Arena *arena);

// Bitmasks
typedef struct {
  u64 *bits;
  size_t bytesize;
  u64 bitsize;
  u64 size;
} Bitmask;

#define bitmaskEquals(a, b) \
  (memcmp((a).bits, (b).bits, (a).bytesize) == 0)

#define addBit(mask, bit) \
  ((mask).bits[(bit) / 64] |= ((u64)1 << ((bit) % 64)))

#define getBit(mask, bit) \
  (bool)((mask).bits[(bit) / 64] & ((u64)1 << ((bit) % 64)))

#define removeBit(mask, bit) \
  ((mask).bits[(bit) / 64] &= ~((u64)1 << ((bit) % 64)))

bool bitmaskContains(Bitmask *mask, Bitmask *element);

void bitmaskInit(Arena *arena, Bitmask *mask, u64 bitsize);
void bitmaskInitCpy(Arena *arena, Bitmask *mask, Bitmask *copying);
void bitmaskPrint(Bitmask *mask);
u32 bitmaskFlagCount(Bitmask *mask);
u32 bitmaskLowestFlag(Bitmask *mask);
u32 bitmaskHighestFlag(Bitmask *mask);
void bitmaskClear(Bitmask *mask);

#endif
