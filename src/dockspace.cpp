#include "../punktop.h"
#include "statsfornerds.h"

void ShowDockSpace(bool &p_open) {
  static ImGuiDockNodeFlags dockspace_flags =
      ImGuiDockNodeFlags_PassthruCentralNode;

  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  window_flags |=
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
    window_flags |= ImGuiWindowFlags_NoBackground;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("DockSpace", nullptr, window_flags);
  ImGui::PopStyleVar();
  ImGui::PopStyleVar(2);

  // DockSpace
  ImGuiIO &io = ImGui::GetIO();
  if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    static auto first_time = true;
    if (first_time) {
      first_time = false;

      // Start StatsForNerds threads
      NerdStats::StartStatsForNerdsThreads();

      ImGui::DockBuilderRemoveNode(dockspace_id); // Clear any previous layout
      ImGui::DockBuilderAddNode(dockspace_id,
                                dockspace_flags | ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

      ImGuiID dock_main_id = dockspace_id;
      ImGui::DockBuilderDockWindow("SpaceY Inc.", dock_main_id);
      ImGui::DockBuilderDockWindow("Sizif-9 Rocket Telemetry", dock_main_id);
      ImGui::DockBuilderDockWindow("Proc List", dock_main_id);
      ImGui::DockBuilderDockWindow("Stats for Nerds", dock_main_id);
      ImGui::DockBuilderFinish(dockspace_id);
      ImGui::SetWindowFocus("Proc List"); // Set focus to Proc List first
    }
  }

  ImGui::End();

  {
    ImGui::Begin("SpaceY Inc.");
    ImGui::Text("Hello from basementq!");
    ImVec2 region = ImGui::GetContentRegionAvail();
    float split_ratio = 0.5f;
    float child_width = region.x * split_ratio;
    float child_height = region.y;
    float panel_height = child_height * 0.5f;
    // Left side
    ImGui::BeginChild(
        "LeftChild", ImVec2(region.x - child_width - 5, child_height), true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::Text("Disk Window");
    ShowNetworkUsage(panel_height);
    ShowDiskUsage();
    ImGui::EndChild();
    ImGui::SameLine();
    // Right side
    ImGui::BeginChild("RightChild", ImVec2(child_width, child_height), true,
                      ImGuiWindowFlags_NoScrollbar |
                          ImGuiWindowFlags_NoScrollWithMouse);
    ShowMemoryUsage(panel_height);
    ShowCpuPlot(panel_height);
    ImGui::EndChild();
    ImGui::End();
  }

  {
    ImGui::Begin("Sizif-9 Rocket Telemetry");
    ImGui::Text("Tonight we steal the moon!");
    ImVec2 region = ImGui::GetContentRegionAvail();
    float child_width = region.x * 0.6f;
    float child_height = region.y * 0.6f;
    ImVec2 cursor_pos = ImGui::GetCursorPos();
    ImGui::SetCursorPosX(cursor_pos.x + (region.x - child_width) * 0.5f);
    ImGui::SetCursorPosY(cursor_pos.y + (region.y - child_height) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Border,
                          IM_COL32(255, 100, 100, 255)); // Red border
    ImGui::BeginChild("CpuUsageChild", ImVec2(child_width, child_height), true,
                      ImGuiChildFlags_Borders);
    static SystemInfo sysinfo = ReadSystemInfo();
    ShowSystemInfo(sysinfo);
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::End();
  }

  {
    ImGui::Begin("Proc List");
    const float top_height_ratio = 0.55f;
    const float left_width_ratio = 0.5f;
    ImVec2 region = ImGui::GetContentRegionAvail();
    float top_height = region.y * top_height_ratio;
    float bottom_height = region.y - top_height;
    float left_width = region.x * left_width_ratio;
    float right_width = region.x - left_width;
    ImGui::BeginChild("ProcTop", ImVec2(region.x, top_height), true);
    ImGui::Text("Top (Process List)");
    ReadMemInfo();
    ImGui::SameLine();
    const char *sortItems[] = {
        "no_sort",  "name_sort",   "name_desc",   "pid_sort",
        "pid_desc", "mem_sort",    "mem_desc",    "cpu_sort",
        "cpu_desc", "thread_sort", "thread_desc",
    };
    static int currentItem = 1;
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::Combo("##SortBy", &currentItem, sortItems,
                     IM_ARRAYSIZE(sortItems))) {
      sortMode = static_cast<SortMode>(currentItem);
    }
    ImGui::SameLine();
    ImGui::Text("Sort");
    static char searchBuffer[64] = "";
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::InputTextWithHint("##Search", "Search process...", searchBuffer,
                                 IM_ARRAYSIZE(searchBuffer))) {
      search_query = searchBuffer;
    }
    ShowProcessesV();
    ImGui::EndChild();
    ImGui::BeginChild("ProcBottom", ImVec2(region.x, bottom_height), false);
    ImGui::BeginChild("ProcBottomLeft", ImVec2(left_width, bottom_height),
                      true);
    ImGui::Text("Left (Network Window)");
    ShowNetworkUsage(0);
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("ProcBottomRight", ImVec2(right_width, bottom_height),
                      true);
    ImGui::Text("Right (CPU Window)");
    if (!is_finished)
      ShowCpuUsage();
    ImGui::EndChild();
    ImGui::EndChild();
    ImGui::End();
  }

  {
    NerdStats::ShowNerdStats();
  }
}
