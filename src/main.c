#include "ecs/ecs.h"
#include <raylib.h>
#include <raymath.h>

typedef struct {
  float x, y;
} Position, Move;
USING_COMPONENT(Position);
USING_COMPONENT(Move);

const int SCREEN_WIDTH = 960, SCREEN_HEIGHT = 540;
int main() {
  printf("Program is running...\n");
  ecsInit(7 MB);
  Scene scene;
  sceneInit(&scene);
  setCurrentScene(&scene);

  for (u32 i = 0; i < 10000; i++) {
    EntityID ent = newEntity();
    addComponent(ent, Position);
    setComponent(ent, Position, {
      .x = GetRandomValue(0, SCREEN_WIDTH),
      .y = GetRandomValue(0, SCREEN_HEIGHT)
    });

    addComponent(ent, Move);
    setComponent(ent, Move, {
      .x = GetRandomValue(-100, 100),
      .y = GetRandomValue(-100, 100)
    });
  }

  ECSQuery position_query;
  queryInit(&position_query);
  queryRequire(&position_query, Position);

  ECSQuery move_sys_query;
  queryInit(&move_sys_query);
  queryRequire(&move_sys_query, Position);
  queryRequire(&move_sys_query, Move);

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "cirkul!");

  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BLACK);

    float delta = GetFrameTime();

    char fps_buff[16];
    sprintf(fps_buff, "%i", GetFPS());
    SetWindowTitle(fps_buff);
     
    queryForeach(&move_sys_query, entity, {
      Position *pos = getComponent(entity, Position);
      Move *move = getComponent(entity, Move);
      pos->x += move->x * delta;
      pos->y += move->y * delta;

      if (pos->x < 0 || pos->x > SCREEN_WIDTH) {
        move->x *= -1;
        pos->x += move->x * delta;
      }
      if (pos->y < 0 || pos->y > SCREEN_HEIGHT) {
        move->y *= -1;
        pos->y += move->y * delta;
      }
    });
    
    // Draw sys
    queryForeach(&position_query, entity, {
      Position *pos = getComponent(entity, Position);
      DrawPixel(pos->x, pos->y, WHITE);
    });
    
    Vector2 mouse_pos = GetMousePosition();
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      queryForeach(&position_query, entity, {
        Position *pos = getComponent(entity, Position);
        if (Vector2Distance((Vector2) {pos->x, pos->y}, mouse_pos) < 32) {
          killEntity(entity);
        }
      });
    }
    
    EndDrawing();
  }

  CloseWindow();
  ecsDeinit();
  return 0;
}
