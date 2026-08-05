#define NO_FONT_AWESOME
#include <iostream>
#include "raylib.h"
#include "imgui.h"
#include "rlImGui.h"

int main()
{
  InitWindow(1600, 900, "box");
  while(!WindowShouldClose()){
    BeginDrawing();
    ClearBackground(RAYWHITE);
    EndDrawing();
  }
  CloseWindow();
  return 0;
}
