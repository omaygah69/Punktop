#include <iostream>
#include "raylib.h"

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
