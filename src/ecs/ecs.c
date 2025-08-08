#include "ecs.h"

// Globals
size_t component_sizes[MAX_COMPONENTS] = {0};
Scene *current_scene = NULL;
Bitmask lcl_bitmask;
Arena ecs_arena;

// Archetypes
static void archetypeInit(Arena *arena, Archetype *type, Bitmask mask) {
  u32 component_count = bitmaskFlagCount(&mask);
  u8 lowest_component_id = bitmaskLowestFlag(&mask);
  u8 component_id_range = bitmaskHighestFlag(&mask) - lowest_component_id + 1;

  *type = (Archetype) {
    .arena = arena,
    .component_arrays = arenaAlloc(arena, sizeof(void*) * component_count),
    .entity_index = arenaAlloc(arena, sizeof(u32) * MAX_ENTITIES),
    .component_id = arenaAlloc(arena, sizeof(ComponentID) * component_count),
    .component_index = arenaAlloc(arena, sizeof(u8) * component_id_range),
    .entities = NULL,

    .component_count = component_count,
    .lowest_component_id = lowest_component_id,
    .cap = 0,
    .size = 0
  };
  
  u8 i = 0, component_id = 0;
  while (i < component_count) {
    if (getBit(mask, component_id)) {
      type->component_id[i] = component_id;
      type->component_index[component_id - lowest_component_id] = i;
      i++;
    }
    component_id++;
  }
  
  bitmaskInitCpy(arena, &type->component_mask, &mask);
}

static void archetypeResize(Archetype *type) {
  Arena *arena = type->arena;

  size_t old_cap = type->cap;
  if (old_cap) {
    type->cap *= 2;
  } else {
    type->cap = 1;
  }
  
  // Copying over required
  if (old_cap) {
    ComponentID component_id;
    for (u8 i = 0; i < type->component_count; i++) {
      component_id = type->component_id[i];

      void *new_arr = arenaAlloc(
        arena, component_sizes[component_id] * type->cap);
      
      memcpy(
        new_arr, type->component_arrays[i],
        component_sizes[component_id] * old_cap);

      type->component_arrays[i] = new_arr;
    }
    EntityID *new_arr = arenaAlloc(arena, sizeof(EntityID) * type->cap);
    memcpy(new_arr, type->entities, sizeof(EntityID) * old_cap);
    type->entities = new_arr;

  // Copying over not required
  } else {
    ComponentID component_id;
    for (u8 i = 0; i < type->component_count; i++) {
      component_id = type->component_id[i];

      type->component_arrays[i] = arenaAlloc(
        arena, component_sizes[component_id] * type->cap);
    }
    type->entities = arenaAlloc(arena, sizeof(EntityID) * type->cap);
  }
}

static void archetypeRemoveEntity(Archetype *type, EntityID entity) {
  // Overwrite all data by last entity and decrement type->size
  u32 to_index = type->entity_index[entity];
  u32 from_index = type->size - 1; // Last entity
  EntityID from_entity = type->entities[from_index];

  // Last entity, no need for moving memory
  if (to_index == from_index) {
    type->size--;
    return;
  }

  for (u8 i = 0; i < type->component_count; i++) {
    void *component_arr = type->component_arrays[i];
    ComponentID component_id = type->component_id[i];
    size_t component_size = component_sizes[component_id];


    memcpy(
      component_arr + component_size * to_index,
      component_arr + component_size * from_index,
      component_size);
  }
  type->entities[to_index] = type->entities[from_index];
  type->entity_index[from_entity] = to_index;
  type->size--;
}

static u8 archetypeGetComponentIndex(Archetype *type, ComponentID id) {
  return type->component_index[id - type->lowest_component_id];
}

static void archetypeInsertEntityID(Archetype *type, EntityID entity) {
  if (type->size >= type->cap) {
    archetypeResize(type);
  }
  type->entities[type->size] = entity;
  type->entity_index[entity] = type->size;

  type->size++;
}

static void archetypeMoveEntity(Archetype *from, Archetype *to, EntityID entity) {
  archetypeInsertEntityID(to, entity);
  
  u32 entity_index_from = from->entity_index[entity],
      entity_index_to = to->entity_index[entity];

  // Copy over shared components (function assumes the bigger type has all the components of smaller type)
  Archetype *smaller_type = from->component_count < to->component_count ?
    from : to;

  for (u8 i = 0; i < smaller_type->component_count; i++) {
    ComponentID component_id = smaller_type->component_id[i];
    u8 comp_index_from = archetypeGetComponentIndex(from, component_id);
    u8 comp_index_to = archetypeGetComponentIndex(to, component_id);
    size_t comp_size = component_sizes[component_id];

    memcpy(
      to->component_arrays[comp_index_to] + comp_size * entity_index_to,
      from->component_arrays[comp_index_from] + comp_size * entity_index_from,
      comp_size);
  }
  
  // Remove entity from old type
  archetypeRemoveEntity(from, entity);
}

// Scenes
void sceneInit(Scene *scene) {
  (*scene) = (Scene) {
    .arena = &ecs_arena,
    .id_queue_tail = 0,
    .id_queue_head = 0,
    .type_count = 0,
    .max_entity_id = 0
  };

  for (u32 i = 0; i < MAX_ARCHETYPES; i++) {
    scene->type_map[i] = NULL;
  }
  for (u32 i = 0; i < MAX_ENTITIES; i++) {
    scene->entity_type[i] = NULL;
  }

}

EntityID newEntity() {
  // Queue empty
  if (current_scene->id_queue_head == current_scene->id_queue_tail) {
    return current_scene->max_entity_id++;
  }

  current_scene->id_queue_tail++;
  return current_scene->id_queue[(current_scene->id_queue_tail - 1) % MAX_FREE_IDS];
}

static Archetype *createArchetype(Scene *scene, Bitmask mask) {
  Archetype *type = &scene->types[scene->type_count];
  archetypeInit(scene->arena, type, mask);
  
  // Insert into map
  u64 hash = mask.bits[0];
  while (scene->type_map[hash % MAX_ARCHETYPES]) {
    hash++;
  }

  scene->type_map[hash] = type;
  scene->type_count++;
  return type;
}

static Archetype *getOrCreateArchetype(Scene *scene, Bitmask mask) {
  // Check if archetype is in map
  u32 hash = mask.bits[0];
  Archetype *type;

  while ((type = scene->type_map[hash % MAX_ARCHETYPES])) {
    // Found the type, return it
    if (bitmaskEquals(type->component_mask, mask)) {
      return type;
    }
    // Continue search
    hash++;
  }
  return createArchetype(scene, mask);
}

void _addComponent(Scene *scene, EntityID entity, ComponentID component_id) {
  Archetype *old_type = scene->entity_type[entity];
  
  // Copy old type component mask if there is one
  if (old_type) {
    memcpy(
      lcl_bitmask.bits, old_type->component_mask.bits,
      lcl_bitmask.bytesize);

  // No previous type, empty
  } else {
    memset(lcl_bitmask.bits, 0, lcl_bitmask.bytesize);
  }
  addBit(lcl_bitmask, component_id);

  Archetype *new_type = getOrCreateArchetype(scene, lcl_bitmask);
  
  // Move type from old to new if needed
  if (old_type) {
    archetypeMoveEntity(old_type, new_type, entity);
  
  } else {
    archetypeInsertEntityID(new_type, entity);
  }
  scene->entity_type[entity] = new_type;
}

void *_getComponent(Scene *scene, EntityID entity, ComponentID component_id) {
  Archetype *type = scene->entity_type[entity];

  u8 comp_index = archetypeGetComponentIndex(type, component_id);
  void *comp_arr = type->component_arrays[comp_index];
  
  size_t component_size = component_sizes[component_id];
  u32 entity_index = type->entity_index[entity];

  return comp_arr + component_size * entity_index;
}

void killEntity(EntityID entity) {
  archetypeRemoveEntity(current_scene->entity_type[entity], entity);
  current_scene->entity_type[entity] = NULL;
}

void setCurrentScene(Scene *scene) {
  current_scene = scene;
}

Scene *getCurrentScene() {
  return current_scene;
}

// Queries
void queryInit(ECSQuery *query) {
  (*query) = (ECSQuery) {
    .scene = current_scene,
  };
  bitmaskInit(current_scene->arena, &query->mask, MAX_COMPONENTS);
}

void _queryRequire(ECSQuery *query, ComponentID component_id) {
  addBit(query->mask, component_id);
}


// Init/deinit
void ecsInit(size_t arena_byte_size) {
  arenaInit(&ecs_arena, arena_byte_size);
  bitmaskInit(&ecs_arena, &lcl_bitmask, MAX_COMPONENTS);
}

void ecsDeinit() {
  arenaFree(&ecs_arena);
}
