#include "../punktop.h"

void ShowDockSpace(bool& p_open){
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;


    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);

    // DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        
            static auto first_time = true;
            if (first_time)
                {
                    first_time = false;

                    ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
                    ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
                    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

                    // split the dockspace into 2 nodes -- DockBuilderSplitNode takes in the following args in the following order
                    //   window ID to split, direction, fraction (between 0 and 1), the final two setting let's us choose which id we want (which ever one we DON'T set as NULL, will be returned by the function)
                    //                                                              out_id_at_dir is the id of the node in the direction we specified earlier, out_id_at_opposite_dir is in the opposite direction
                    // auto dock_id_up = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, 0.3f, nullptr, &dockspace_id);
                    // auto dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.3f, nullptr, &dockspace_id);
                    // auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.3f, nullptr, &dockspace_id);

                    ImGuiID dock_main_id = dockspace_id;
                    ImGui::DockBuilderDockWindow("SpaceY Inc.", dock_main_id);
                    ImGui::DockBuilderDockWindow("Sizif-9 Rocket Telemetry", dock_main_id);
                    ImGui::DockBuilderDockWindow("Proc List", dock_main_id);
                    ImGui::DockBuilderFinish(dockspace_id);
                    ImGui::SetWindowFocus("Proc List"); // set focus to proc list firsst
                }
        }

    ImGui::End();

    {
        ImGui::Begin("SpaceY Inc.");
        ImGui::Text("Hello from basementq!");
        ImVec2 region = ImGui::GetContentRegionAvail();
        float split_ratio   = 0.5f;
        float child_width   = region.x * split_ratio;
        float child_height  = region.y;
        // Left side
        ImGui::BeginChild("LeftChild", ImVec2(region.x - child_width - 5, child_height), true);
        ImGui::Text("Network Window");
        ImGui::EndChild();
        ImGui::SameLine();
        // Right side
        ImGui::BeginChild("RightChild", ImVec2(child_width, child_height), true);
        // ShowDiskWindow();  
        ImGui::EndChild();
        ImGui::End();
    }

    {
        ImGui::Begin("Sizif-9 Rocket Telemetry");
        ImGui::Text("Tonight We steal the moon!");
        ImVec2 region = ImGui::GetContentRegionAvail();
        float child_width = region.x * 0.6f;  // 80% of parent width
        float child_height = region.y * 0.6f; // 80% of parent height
        // Centering 
        ImVec2 cursor_pos = ImGui::GetCursorPos();
        ImGui::SetCursorPosX(cursor_pos.x + (region.x - child_width) * 0.5f);
        ImGui::SetCursorPosY(cursor_pos.y + (region.y - child_height) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 100, 100, 255)); // Red border
        ImGui::BeginChild("CpuUsageChild", ImVec2(child_width, child_height), true, ImGuiChildFlags_Borders);
        // if(!is_finished)
        //     ShowCpuUsage();
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::End();
    }
    
    {
        ImGui::Begin("Proc List");
        // Fixed layout ratios
        const float top_height_ratio = 0.55f;
        const float left_width_ratio = 0.5f;
        ImVec2 region = ImGui::GetContentRegionAvail();
        float top_height = region.y * top_height_ratio;
        float bottom_height = region.y - top_height;
        float left_width = region.x * left_width_ratio;
        float right_width = region.x - left_width;
        // --- Top child window ---
        ImGui::BeginChild("ProcTop", ImVec2(region.x, top_height), true);
        ImGui::Text("Top (e.g. summary/stats)");
        ImGui::EndChild();
        // --- Bottom container ---
        ImGui::BeginChild("ProcBottom", ImVec2(region.x, bottom_height), false);
        // --- Left pane ---
        ImGui::BeginChild("ProcBottomLeft", ImVec2(left_width, bottom_height), true);
        ImGui::Text("Left (e.g. process list)");
        ImGui::EndChild();
        ImGui::SameLine();
        // --- Right pane ---
        ImGui::BeginChild("ProcBottomRight", ImVec2(right_width, bottom_height), true);
        ImGui::Text("Right (e.g. details/memory info)");
        ImGui::EndChild();
        ImGui::EndChild(); // End of ProcBottom
    }
    
    ImGui::Text("This is elon Musk!");
    // ReadMemInfo();
    // FetchProcesses();
    // ShowProcesses();
    ImGui::End();
}
