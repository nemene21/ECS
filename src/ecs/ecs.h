#ifndef ECS_H
#define ECS_H
#include "ecs/utils.h"

#define MAX_ARCHETYPES 256
#define MAX_COMPONENTS 128
#define MAX_ENTITIES 10000
#define MAX_FREE_IDS 1024


typedef u32 EntityID;
typedef u16 ComponentID;


// ComponentID -> size bank
extern size_t component_sizes[MAX_COMPONENTS];

#define USING_COMPONENT(TypeName) \
  const ComponentID TypeName##ID = __COUNTER__

#define registerComponentSize(TypeName) \
  component_sizes[TypeName##ID] = sizeof(TypeName)

// Archetypes
typedef struct {
  Bitmask component_mask;

  Arena *arena;
  EntityID *entities;
  u32 *entity_index;

  void **component_arrays;
  ComponentID *component_id;
  u8 *component_index;

  u64 size, cap;
  
  ComponentID lowest_component_id;
  u8 component_count;
} Archetype;

// Scene
typedef struct {
  Arena *arena;
  Archetype types[MAX_ARCHETYPES];
  Archetype *type_map[MAX_ARCHETYPES];
  Archetype *entity_type[MAX_ENTITIES];
  
  EntityID max_entity_id;
  u64 id_queue_tail, id_queue_head;
  EntityID id_queue[MAX_FREE_IDS];
  u32 type_count;
} Scene;

void _addComponent(Scene *scene, EntityID entity, ComponentID id);
void *_getComponent(Scene *scene, EntityID entity, ComponentID id);

EntityID newEntity();
void killEntity(EntityID entity);

#define addComponent(entity, TypeName) \
  registerComponentSize(TypeName); \
  _addComponent(getCurrentScene(), entity, TypeName##ID)

#define getComponent(entity, TypeName) \
  (TypeName*)_getComponent(getCurrentScene(), entity, TypeName##ID)

#define setComponent(entity, TypeName, ...) \
  (*(TypeName*)_getComponent(getCurrentScene(), entity, TypeName##ID)) = (TypeName)__VA_ARGS__

void sceneInit(Scene *scene);

// Scene swapping
void setCurrentScene(Scene *to);
Scene *getCurrentScene();

// Queries
typedef struct {
  Bitmask mask;
  Scene *scene;
} ECSQuery;

void queryInit(ECSQuery *query);
void _queryRequire(ECSQuery *query, ComponentID component_id);

#define queryRequire(queryPtr, CompType) _queryRequire(queryPtr, CompType##ID)

#define queryForeach(queryPtr, entity_out, ...) \
  for (i64 i = 0; i < (queryPtr)->scene->type_count; i++) { \
    Archetype *type = (queryPtr)->scene->types + i; \
    \
    if (bitmaskContains(&type->component_mask, &(queryPtr)->mask)) { \
      for (u64 j = 0; j < type->size; j++) { \
        EntityID entity_out = type->entities[j]; \
        __VA_ARGS__ \
        /* Entity changed archetype, compensate */ \
        if ((queryPtr)->scene->entity_type[entity_out] != type) { j--; } \
      } \
    } \
  }

// Init/deinit
void ecsInit(size_t arena_byte_size);
void ecsDeinit();

#endif
