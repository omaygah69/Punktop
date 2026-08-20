#include "raylib.h"
#include "raymath.h"
#include "imgui.h"
#include "rlImGui.h"
#include "implot.h" 
#include "implot_internal.h" 
#include "punktop.h"
#include "api.h"
#include <iostream>
#include <stdio.h>

int main() {
  std::cout << "[INFO] Running...\n";
  std::thread cpu_worker(GetCpuUsage);
  cpu_worker.detach();
  std::thread net_worker(GetNetworkUsage, "wlp2s0");
  net_worker.detach();
  std::thread mem_worker(FetchMemoryUsage);
  mem_worker.detach();
  std::thread disk_worker(FetchDiskUsage);
  disk_worker.detach();
  std::thread cpu_plot_worker(FetchCpuUsageForPlot);
  cpu_plot_worker.detach();
  std::thread system_json_worker(WriteSystemJson);

  // FLAGS: FLAG_WINDOW_HIGHDPI |
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
  InitWindow(1280, 720, "punktop");
  SetTargetFPS(60);

  rlImGuiSetup(true); // creates ImGui context + hooks raylib backend, true = dark theme
  ImPlot::CreateContext(); // 

  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  Color clear_color = {115, 140, 153, 255}; // ~ (0.45, 0.55, 0.60, 1.0)
  bool dock_window = true;
  double last_update_time = 0.0;

  while (!WindowShouldClose()) {
    if (IsWindowState(FLAG_WINDOW_MINIMIZED)) {
      continue;
    }

    double current_time = GetTime();
    if (current_time - last_update_time >= 2.0) {
      FetchProcesses();
      last_update_time = current_time;
    }

    BeginDrawing();
    ClearBackground(clear_color);

    rlImGuiBegin();
    if (dock_window)
      ShowDockSpace(dock_window);
    rlImGuiEnd();

    EndDrawing();
  }

  is_finished = true; // still relied on by the detached workers
  system_json_worker.join(); // WriteSystemJson loops while(!is_finished)


  rlImGuiShutdown();
  ImPlot::DestroyContext(); 
  CloseWindow();

  return 0;
}
