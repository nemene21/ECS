#include "ecs/ecs.h"
#include <raylib.h>
#include <raymath.h>

typedef struct {
  float x, y;
} Position, Move;

USING_COMPONENT(Position);
USING_COMPONENT(Move);
USING_COMPONENT(Color);

const int SCREEN_WIDTH = 960, SCREEN_HEIGHT = 540;

EntityID newDot(float x, float y) {
  EntityID entity = newEntity();
  addComponent(entity, Position);
  setComponent(entity, Position, {
      .x = x, .y = y});

  addComponent(entity, Color);
  setComponent(entity, Color, {
    GetRandomValue(0, 255), GetRandomValue(0, 255), GetRandomValue(0, 255), 255});

  addComponent(entity, Move);
  setComponent(entity, Move, {
      .x = GetRandomValue(-100, 100), .y = GetRandomValue(-100, 100)});
  
  return entity;
}

ECSQuery draw_query;
ECSSystem draw_system;

void drawSystemStep() {
  Position *pos = getComponentArray(Position);
  Color *col = getComponentArray(Color);
  
  for (u64 i = 0; i < getEntityArraySize(); i++) {
    DrawPixel(pos[i].x, pos[i].y, col[i]);
  }
}

void drawSystemInit() {
  queryInit(&draw_query);
  queryRequire(&draw_query, Position);
  queryRequire(&draw_query, Color);

  draw_system = (ECSSystem) {
    .query = &draw_query,
    .begin = NULL,
    .step = drawSystemStep
  };
}

ECSQuery move_query;
ECSSystem move_system;

void moveSystemStep() {
  float delta = GetFrameTime();

  Position *pos = getComponentArray(Position);
  Move *move = getComponentArray(Move);
  
  for (u64 i = 0; i < getEntityArraySize(); i++) {
    pos[i].x += move[i].x * delta;
    pos[i].y += move[i].y * delta;
  }
}

void moveSystemInit() {
  queryInit(&move_query);
  queryRequire(&move_query, Position);
  queryRequire(&move_query, Move);

  move_system = (ECSSystem) {
    .query = &move_query,
    .begin = NULL,
    .step = moveSystemStep
  };
}

int main() {
  ecsInit(20 MB);
  Scene scene;
  sceneInit(&scene);
  setCurrentScene(&scene);

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "cirkul!");
  drawSystemInit();
  moveSystemInit();

  for (u64 i = 0; i < 1000; i++) {
    newDot(GetRandomValue(0, SCREEN_WIDTH), GetRandomValue(0, SCREEN_HEIGHT));
  }

  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BLACK);

    char fps_buff[16];
    sprintf(fps_buff, "%i", GetFPS());
    SetWindowTitle(fps_buff);

    runSystem(&draw_system);
    runSystem(&move_system);

    EndDrawing();
  }

  CloseWindow();
  ecsDeinit();
  return 0;
}
