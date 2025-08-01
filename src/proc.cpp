#include <stdio.h>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <cstring>
#include "../punktop.h"
#include <unistd.h>
#include "imgui.h"

namespace fs = std::filesystem;
static const fs::path dir_path = "/proc/";
static std::vector<Process> Procs;
static std::mutex mtx;

void ShowProcesses()
{
    ImGui::BeginChild("ProcScroll", ImVec2(0, 400), true); // Scrollable 
    if (ImGui::BeginTable("ProcTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Memory (MB)", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableHeadersRow();

        for(const Process proc : Procs)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%s", proc.Pid.c_str());
            ImGui::TableNextColumn(); ImGui::Text("%s", proc.Name.empty() ?  "{Unknown}" : proc.Name.c_str());
            ImGui::TableNextColumn();

            if (proc.MemUsage >= 0)
            {
                float memMB = proc.MemUsage / 1024.0f;
                if (memMB > 200.0f)
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%.2f MB", memMB);
                else if(memMB > 100.0f)
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.2f MB", memMB); 
                else
                    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%.2f MB", memMB); 
            }
            else
            {
                ImGui::Text("0.0B");
            }
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}

void FetchProcesses()
{
    Procs.clear();
    try {
        if (fs::exists(dir_path) && fs::is_directory(dir_path))
        {
            for(const auto& entry : fs::directory_iterator(dir_path))
            {
                if (fs::is_directory(entry.status()))
                {
                    std::string entrypid = entry.path().filename().string();
                    const char* entryp_path = entry.path().c_str();

                    if(IsNumeric(entrypid))
                    {
                        char* name = GetProcName(entryp_path);
                        int memory = GetMemoryUsage(entryp_path);

                        Process proc;
                        proc.Pid = entrypid;
                        proc.Name = name ? name : "Unknown";
                        proc.MemUsage = memory;

                        Procs.push_back(proc);
                        if(name)
                            free(name);
                    }
                }
            }
        }
        else
        {
            std::cerr << "[ERROR] Directory does not exist.\n";
        }
    }
    catch (const fs::filesystem_error& error)
    {
        std::cerr << "[ERROR] Filesystem Error: " << error.what() << "\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ERROR] " << e.what() << "\n";
    }
}

bool IsNumeric(std::string dir_name) {
    char* buffer;
    std::strtol(dir_name.c_str(), &buffer, 10);
    return *buffer == 0;
}

char* GetProcName(const char* path) {
    char status_file[64];
    snprintf(status_file, sizeof(status_file), "%s/status", path);
    FILE* pf = fopen(status_file, "r");
    if (pf == NULL) {
        return NULL;
    }
    char buffer[64];
    char* name = NULL;
    if (fgets(buffer, sizeof(buffer), pf)) {
        if (strncmp(buffer, "Name:", 5) == 0) {
            name = strdup(buffer + 6); // Copy string after "Name: "
            name[strcspn(name, "\n")] = '\0'; // Trim newline
        }
    }

    fclose(pf);
    return name;
}

void ReadMemInfo() {
    // const char* mem_info = "/proc/meminfo";
    FILE* pf = fopen("/proc/meminfo", "r");
    if (pf == nullptr) {
        printf("[ERROR] Failed opening file\n");
        return;
    }
    char buffer[64];
    float memory = 0;
    float memMB = 0;
    while(fgets(buffer, sizeof(buffer), pf)){
        if(strncmp(buffer, "Active:", 7) == 0) {
            sscanf(buffer + 7, "%f", &memory);  
            memMB = memory / 1024.0f;
            break;
        }
    }
    // ImGui::Text("[INFO] Total Memory by kb: %.2f", memory);
    ImGui::Text("[INFO] Total Memory: %.2fMB", memMB);
    fclose(pf);
}

int GetMemoryUsage(const char* path) {
    char status_file[64];
    snprintf(status_file, sizeof(status_file), "%s/status", path);

    FILE* pf = fopen(status_file, "r");
    if (pf == nullptr) {
        fprintf(stderr, "[ERROR] Failed opening file\n");
        return -1;
    }

    char line[64];
    int memory = -1;

    while (fgets(line, sizeof(line), pf)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line + 6, "%d", &memory);  // Skip past "VmRSS:"
            // printf("[INFO] %s", line); 
            break;
        }
    }
    fclose(pf);
    return memory;
}
