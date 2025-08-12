void ShowProcessesV()
{
    ImGui::BeginChild("ProcScroll", ImVec2(0, 400), true, ImGuiWindowFlags_NoScrollbar);

    static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("ProcTable", 4, flags))
        {
            ImGui::TableSetupScrollFreeze(0, 1); // Freeze top row (header)
            ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("User", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            /* ImGui::TableSetupColumn("%CPU", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort, 60.0f); */
            ImGui::TableSetupColumn("Mem (MB)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            /* ImGui::TableSetupColumn("Threads", ImGuiTableColumnFlags_WidthFixed, 60.0f); */
            /* ImGui::TableSetupColumn("Command", ImGuiTableColumnFlags_WidthStretch); */
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin(Procs.size());
            while (clipper.Step())
                {
                    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                        {
                            const Process& proc = Procs[i];
                            ImGui::TableNextRow();

                            // PID
                            ImGui::TableNextColumn();
                            ImGui::Text("%s", proc.Pid.c_str());

                            // User
                            ImGui::TableNextColumn();
                            ImGui::Text("%s", proc.User.c_str());

                            // Name
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(proc.Name.empty() ? "{Unknown}" : proc.Name.c_str());

                            // CPU
                            /* ImGui::TableNextColumn(); */
                            /* if (proc.CpuUsage > 50.0f) */
                            /*     ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%.1f", proc.CpuUsage); */
                            /* else if (proc.CpuUsage > 20.0f) */
                            /*     ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.1f", proc.CpuUsage); */
                            /* else */
                            /*     ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%.1f", proc.CpuUsage); */

                            // Memory
                            ImGui::TableNextColumn();
                            float memMB = proc.MemUsage / 1024.0f;
                            if (memMB > 200.0f)
                                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%.1f", memMB);
                            else if (memMB > 100.0f)
                                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.1f", memMB);
                            else
                                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%.1f", memMB);

                            // Threads
                            /* ImGui::TableNextColumn(); */
                            /* ImGui::Text("%d", proc.Threads); */

                            // Command
                        /*     ImGui::TableNextColumn(); */
                        /*     ImGui::TextUnformatted(proc.Command.c_str()); */
                        }
                }
            ImGui::EndTable();
        }
    ImGui::EndChild();
}
