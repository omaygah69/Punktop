#include <stdio.h>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <cstring>
#include "../punktop.h"
#include <unistd.h>
#include <signal.h>
#include <algorithm>

namespace fs = std::filesystem;
static const fs::path dir_path = "/proc/";
static std::vector<Process> Procs;
static std::mutex mtx;
static std::string selected_pid;
SortMode sortMode = no_sort;

void ShowProcessesV()
{
    ImGui::BeginChild("ProcScroll", ImVec2(0, 400), true,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    static ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingStretchProp |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Sortable |
        ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("ProcTable", 7, flags))
    {
        ImGui::TableSetupScrollFreeze(0, 1); // Freeze top row
        ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("User", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("%CPU", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Mem (MB)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Threads", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Command");
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin((int)Procs.size());
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
            {
                const Process& proc = Procs[i];
                bool is_selected = (proc.Pid == selected_pid);

                ImGui::TableNextRow();

                // Clickable row start
                ImGui::PushID(i);
                ImGui::TableNextColumn();

                // Detect row click
                if (ImGui::Selectable(proc.Pid.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns))
                {
                    selected_pid = proc.Pid;
                }

                // Context menu
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Kill Process"))
                    {
                        // Kill Proc
                        std::cout << "Killing PID: " << proc.Pid << "\n";
                        KillProc(proc.Pid);
                    }
                    if (ImGui::MenuItem("Pin Process"))
                    {
                        // TODO: add to pinned list
                        std::cout << "Pinning PID: " << proc.Pid << "\n";
                    }
                    ImGui::EndPopup();
                }

                // User
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(proc.User.c_str());

                // Name
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(proc.Name.empty() ? "{Unknown}" : proc.Name.c_str());

                // CPU
                ImGui::TableNextColumn();
                ImVec4 cpu_color = (proc.CpuUsage > 50.0f) ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) :
                                     (proc.CpuUsage > 20.0f) ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f) :
                                                                ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
                ImGui::TextColored(cpu_color, "%.1f", proc.CpuUsage);

                // Memory
                ImGui::TableNextColumn();
                float memMB = proc.MemUsage / 1024.0f;
                ImVec4 mem_color = (memMB > 200.0f) ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) :
                                     (memMB > 100.0f) ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f) :
                                                        ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
                ImGui::TextColored(mem_color, "%.1f", memMB);

                // Threads
                ImGui::TableNextColumn();
                ImGui::Text("%d", proc.ThreadCount);

                // Command
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(proc.Command.c_str());

                ImGui::PopID();
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
                    std::string entry_pid = entry.path().filename().string();
                    const char* entryp_path = entry.path().c_str();

                    if(IsNumeric(entry_pid))
                    {
                        int memory = GetMemoryUsage(entryp_path);

                        Process proc;
                        proc.Pid = entry_pid;
                        proc.Name = GetProcName(entryp_path);
                        proc.MemUsage = memory;
                        proc.User = GetProcUser(entryp_path);
                        proc.Command = GetProcCommand(entryp_path);
                        proc.CpuUsage = GetProcCpuUsage(entry_pid);
                        proc.ThreadCount = GetProcThreadCount(entryp_path);

                        Procs.push_back(proc);
                        // if(name)
                        //     free(name);
                    }
                }
            }
            if(sortMode != no_sort)
                SortProcesses(sortMode);
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

// TODO
void SortProcesses(SortMode mode)
{
    switch(mode){
    case name_sort:
        std::sort(Procs.begin(), Procs.end(),[](const Process& a, Process& b){
            return a.Name < b.Name;
        });
            break;
            
    case name_desc:
        std::sort(Procs.begin(), Procs.end(),[](const Process& a, Process& b){
            return a.Name > b.Name;
        });
        break;

        
    case pid_sort:
        std::sort(Procs.begin(), Procs.end(),[](const Process& a, Process& b){
            return std::stoi(a.Pid) > std::stoi(b.Pid);
        });
        break;

    case pid_desc:
        std::sort(Procs.begin(), Procs.end(),[](const Process& a, Process& b){
            return std::stoi(a.Pid) < std::stoi(b.Pid);
        });
        break;
                        
    case mem_sort:
        std::sort(Procs.begin(), Procs.end(), [](const Process& a, Process& b){
            return a.MemUsage > b.MemUsage;
        });
        break;
        
    case mem_desc:
        std::sort(Procs.begin(), Procs.end(), [](const Process& a, Process& b){
            return a.MemUsage < b.MemUsage;
        });
        break;

    case cpu_sort:
        std::sort(Procs.begin(), Procs.end(),[](const Process& a, Process& b){
            return a.CpuUsage > b.CpuUsage;
        });
        break;

    case cpu_desc:
        std::sort(Procs.begin(), Procs.end(),[](const Process& a, Process& b){
            return a.CpuUsage < b.CpuUsage;
        });
        break;

    case thread_sort:
        std::sort(Procs.begin(), Procs.end(),[](const Process& a, Process& b){
            return a.ThreadCount > b.ThreadCount;
        });
        break;
        
    case thread_desc:
        std::sort(Procs.begin(), Procs.end(),[](const Process& a, Process& b){
            return a.ThreadCount < b.ThreadCount;
        });
        break;
        
    default:
        break;
    }
}

bool IsNumeric(std::string dir_name) {
    char* buffer;
    std::strtol(dir_name.c_str(), &buffer, 10);
    return *buffer == 0;
}

std::string GetProcName(const char* path) {
    char status_file[64];
    snprintf(status_file, sizeof(status_file), "%s/status", path);
    FILE* pf = fopen(status_file, "r");
    if (!pf) return "{Unknown}";
    char line[128];
    std::string name = "{Unknown}";
    while (fgets(line, sizeof(line), pf)) {
        if (strncmp(line, "Name:", 5) == 0) {
            name = std::string(line + 6);
            // trim leading spaces
            name.erase(0, name.find_first_not_of(" \t"));
            // trim trailing newline
            name.erase(name.find_last_not_of("\n") + 1);
            break;
        }
    }
    fclose(pf);
    return name;
}

std::string GetProcCommand(const char* path) {
    char cmd_file[64];
    snprintf(cmd_file, sizeof(cmd_file), "%s/cmdline", path);
    FILE* pf = fopen(cmd_file, "r");
    if (!pf) return "{Unknown}";
    std::string cmd;
    char ch;
    while ((ch = fgetc(pf)) != EOF) {
        cmd += (ch == '\0') ? ' ' : ch;
    }
    fclose(pf);
    return cmd.empty() ? "{Unknown}" : cmd;
}

std::string GetProcUser(const char* path)
{
    struct stat info;
    if(stat(path, &info) != 0)
        return "[WARN] Unknown";
    struct passwd* pw = getpwuid(info.st_uid);
    return pw ? pw->pw_name : "Unknown";
}

int GetProcThreadCount(const char* path) {
    char status_file[64];
    snprintf(status_file, sizeof(status_file), "%s/status", path);
    FILE* pf = fopen(status_file, "r");
    if (!pf) return -1;
    char line[128];
    int threads = -1;
    while (fgets(line, sizeof(line), pf)) {
        if (strncmp(line, "Threads:", 8) == 0) {
            sscanf(line + 8, "%d", &threads);
            break;
        }
    }
    fclose(pf);
    return threads;
}

float GetProcCpuUsage(const std::string& pid){
    char stat_file[64];
    snprintf(stat_file, sizeof(stat_file), "/proc/%s/stat", pid.c_str());
    FILE* pf = fopen(stat_file, "r");
    if (!pf) return 0.0f;
    long utime, stime;
    long clk_tck = sysconf(_SC_CLK_TCK);

    // Extract needed fields from /proc/[pid]/stat file 
    // Format: pid (comm) state ppid ... utime stime ... starttime ...
    char comm[256], state;
    int ppid, pgrp, session, tty_nr, tpgid;
    unsigned flags;
    unsigned long cutime, cstime, priority, nice, num_threads, itrealvalue;
    unsigned long long start_time_ticks;
    unsigned long vsize;
    long rss_val;
    fscanf(pf, "%*d %*s %*c "
               "%*d %*d %*d %*d %*d "
               "%*u %*u %*u %*u %lu %lu "
               "%*d %*d %*d %*d %*d %*d "
               "%llu",
               &utime, &stime, &start_time_ticks);
    fclose(pf);
    long total_time = utime + stime;
    
    // System uptime
    float uptime_secs = 0;
    pf = fopen("/proc/uptime", "r");
    if (!pf) return 0.0f;
    fscanf(pf, "%f", &uptime_secs);
    fclose(pf);

    // Seconds the process has been running
    float seconds = uptime_secs - (start_time_ticks / (float)clk_tck);
    if (seconds <= 0) return 0.0f;
    float cpu_usage = 100.0f * ((total_time / (float)clk_tck) / seconds);
    return cpu_usage;
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
    // ImGui::Text("[INFO] Total Memory: %.2fMB", memMB);
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "[INFO] Total Memory: %.2fMB", memMB);
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
            sscanf(line + 6, "%d", &memory);  // Skip "VmRSS:"
            // printf("[INFO] %s", line); 
            break;
        }
    }
    fclose(pf);
    return memory;
}

void KillProc(std::string proc_pid)
{
    pid_t pid;
    try {
        pid_t pid = std::stoi(proc_pid);
        if(kill(pid, SIGKILL) != 0){
            std::cout << "[ERROR] Failed Killing Task\n";
        }
    }
    catch (const std::invalid_argument &e) {
        std::cerr << "Invalid number\n";
    }
    catch (const std::out_of_range &e) {
        std::cerr << "Number out of range\n";
    }
}
