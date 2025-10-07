#include "../punktop.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;
static const fs::path dir_path = "/proc/";
static std::vector<Process> Procs;
static std::mutex mtx;
static std::string selected_pid;
static int current_match_index;
SortMode sortMode = no_sort;
std::string search_query;
static std::unordered_set<std::string> pinned_pids;

void ShowProcessesV() {
    ImGui::BeginChild("ProcScroll", ImVec2(0, 400), true);

    static ImGuiTableFlags flags =
        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY;

    // Filtered indexes
    std::vector<int> filtered_indexes;
    for (int i = 0; i < (int)Procs.size(); i++) {
        const Process &proc = Procs[i];
        if (search_query.empty() ||
            proc.Name.find(search_query) != std::string::npos ||
            proc.Pid.find(search_query) != std::string::npos ||
            proc.Command.find(search_query) != std::string::npos) {
            filtered_indexes.push_back(i);
        }
    }

    // Split pinned and normal
    std::vector<int> pinned_indexes, normal_indexes;
    for (int idx : filtered_indexes) {
        if (pinned_pids.count(Procs[idx].Pid))
            pinned_indexes.push_back(idx);
        else
            normal_indexes.push_back(idx);
    }

    if (ImGui::BeginTable("ProcTable", 7, flags)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("User", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("%CPU", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Mem (MB)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Threads", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Command");
        ImGui::TableHeadersRow();

        auto render_row = [&](int i) {
            const Process &proc = Procs[i];
            bool is_selected = (proc.Pid == selected_pid);
            bool is_pinned = pinned_pids.count(proc.Pid);

            ImGui::TableNextRow();
            ImGui::PushID(i);
            ImGui::TableNextColumn();

            if (ImGui::Selectable(proc.Pid.c_str(), is_selected,
                                  ImGuiSelectableFlags_SpanAllColumns)) {
                selected_pid = (is_selected ? "" : proc.Pid);
            }

            // Context menu
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem(is_pinned ? "Unpin Process" : "Pin Process")) {
                    if (is_pinned)
                        pinned_pids.erase(proc.Pid);
                    else
                        pinned_pids.insert(proc.Pid);
                }
                if (ImGui::MenuItem("Kill Process")) {
                    KillProc(proc.Pid);
                }
                ImGui::EndPopup();
            }

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(proc.User.c_str());

            ImGui::TableNextColumn();
            if (is_pinned)
                ImGui::TextColored(ImVec4(0.7f, 0.8f, 1.0f, 1.0f), "📌 %s", proc.Name.c_str());
            else
                ImGui::TextUnformatted(proc.Name.c_str());

            ImGui::TableNextColumn();
            ImVec4 cpu_color =
                (proc.CpuUsage > 50.0f) ? ImVec4(1, 0.3f, 0.3f, 1)
                : (proc.CpuUsage > 20.0f) ? ImVec4(1, 1, 0, 1)
                                          : ImVec4(0.3f, 1, 0.3f, 1);
            ImGui::TextColored(cpu_color, "%.1f", proc.CpuUsage);

            ImGui::TableNextColumn();
            float memMB = proc.MemUsage / 1024.0f;
            ImVec4 mem_color =
                (memMB > 200) ? ImVec4(1, 0.3f, 0.3f, 1)
                : (memMB > 100) ? ImVec4(1, 1, 0, 1)
                                : ImVec4(0.3f, 1, 0.3f, 1);
            ImGui::TextColored(mem_color, "%.1f", memMB);

            ImGui::TableNextColumn();
            ImGui::Text("%d", proc.ThreadCount);

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(proc.Command.c_str());
            ImGui::PopID();
        };

        // Render pinned processes first
        for (int i : pinned_indexes)
            render_row(i);

        if (!pinned_indexes.empty())
            ImGui::Separator();

        for (int i : normal_indexes)
            render_row(i);

        ImGui::EndTable();
    }

    ImGui::EndChild();
}

void FetchProcesses() {
    Procs.clear();
    try {
        if (fs::exists(dir_path) && fs::is_directory(dir_path)) {
            for (const auto &entry : fs::directory_iterator(dir_path)) {
                if (fs::is_directory(entry.status())) {
                    std::string entry_pid = entry.path().filename().string();
                    const char *entryp_path = entry.path().c_str();

                    if (IsNumeric(entry_pid)) {
                        int memory = GetMemoryUsage(entryp_path);

                        Process proc;
                        proc.Pid = entry_pid;
                        proc.Name = GetProcName(entryp_path);
                        proc.MemUsage = memory;
                        proc.ParentId = GetProcPpid(entryp_path);
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

            //  build parent-child relationships 
            std::unordered_map<std::string, int> pidToIndex;
            for (int i = 0; i < (int)Procs.size(); i++) {
                pidToIndex[Procs[i].Pid] = i;
                Procs[i].Children.clear(); // reset before rebuilding
            }

            for (int i = 0; i < (int)Procs.size(); i++) {
                auto &proc = Procs[i];
                if (pidToIndex.count(proc.ParentId)) {
                    Procs[pidToIndex[proc.ParentId]].Children.push_back(i);
                }
            }

            if (sortMode != no_sort)
                SortProcesses(sortMode);
        } else {
            std::cerr << "[ERROR] Directory does not exist.\n";
        }
    } catch (const fs::filesystem_error &error) {
        std::cerr << "[ERROR] Filesystem Error: " << error.what() << "\n";
    } catch (const std::exception &e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
    }
}

void SortProcesses(SortMode mode) {
    switch (mode) {
    case name_sort:
        std::sort(Procs.begin(), Procs.end(),
                  [](const Process &a, const Process &b) {
                      return a.Name < b.Name;
                  });
        break;

    case name_desc:
        std::sort(Procs.begin(), Procs.end(),
                  [](const Process &a, const Process &b) {
                      return a.Name > b.Name;
                  });
        break;

    case pid_sort: // ascending
        std::sort(Procs.begin(), Procs.end(),
                  [](const Process &a, const Process &b) {
                      return std::stoi(a.Pid) < std::stoi(b.Pid);
                  });
        break;

    case pid_desc: // descending
        std::sort(Procs.begin(), Procs.end(),
                  [](const Process &a, const Process &b) {
                      return std::stoi(a.Pid) > std::stoi(b.Pid);
                  });
        break;

    case mem_sort: // descending = high usage first
        std::sort(Procs.begin(), Procs.end(),
                  [](const Process &a, const Process &b) {
                      return a.MemUsage > b.MemUsage;
                  });
        break;

    case mem_desc: // ascending
        std::sort(Procs.begin(), Procs.end(),
                  [](const Process &a, const Process &b) {
                      return a.MemUsage < b.MemUsage;
                  });
        break;

    case cpu_sort: // descending high CPU first
        std::sort(Procs.begin(), Procs.end(),
                  [](const Process &a, const Process &b) {
                      return a.CpuUsage > b.CpuUsage;
                  });
        break;

    case cpu_desc: // ascending
        std::sort(Procs.begin(), Procs.end(),
                  [](const Process &a, const Process &b) {
                      return a.CpuUsage < b.CpuUsage;
                  });
        break;

    case thread_sort: // descending
        std::sort(Procs.begin(), Procs.end(),
                  [](const Process &a, const Process &b) {
                      return a.ThreadCount > b.ThreadCount;
                  });
        break;

    case thread_desc: // ascending
        std::sort(Procs.begin(), Procs.end(),
                  [](const Process &a, const Process &b) {
                      return a.ThreadCount < b.ThreadCount;
                  });
        break;

    default:
        break;
    }
}

bool IsNumeric(std::string dir_name) {
    char *buffer;
    std::strtol(dir_name.c_str(), &buffer, 10);
    return *buffer == 0;
}

std::string GetProcName(const char *path) {
    char status_file[64];
    snprintf(status_file, sizeof(status_file), "%s/status", path);
    FILE *pf = fopen(status_file, "r");
    if (!pf)
        return "{Unknown}";
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

std::string GetProcCommand(const char *path) {
    char cmd_file[64];
    snprintf(cmd_file, sizeof(cmd_file), "%s/cmdline", path);
    FILE *pf = fopen(cmd_file, "r");
    if (!pf)
        return "{Unknown}";
    std::string cmd;
    char ch;
    while ((ch = fgetc(pf)) != EOF) {
        cmd += (ch == '\0') ? ' ' : ch;
    }
    fclose(pf);
    return cmd.empty() ? "{Unknown}" : cmd;
}

std::string GetProcUser(const char *path) {
    struct stat info;
    if (stat(path, &info) != 0)
        return "[WARN] Unknown";
    struct passwd *pw = getpwuid(info.st_uid);
    return pw ? pw->pw_name : "Unknown";
}

int GetProcThreadCount(const char *path) {
    char status_file[64];
    snprintf(status_file, sizeof(status_file), "%s/status", path);
    FILE *pf = fopen(status_file, "r");
    if (!pf)
        return -1;
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

float GetProcCpuUsage(const std::string &pid) {
    char stat_file[64];
    snprintf(stat_file, sizeof(stat_file), "/proc/%s/stat", pid.c_str());
    FILE *pf = fopen(stat_file, "r");
    if (!pf)
        return 0.0f;
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
    int f = fscanf(pf,
                   "%*d %*s %*c "
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
    if (!pf)
        return 0.0f;
    fscanf(pf, "%f", &uptime_secs);
    fclose(pf);

    // Seconds the process has been running
    float seconds = uptime_secs - (start_time_ticks / (float)clk_tck);
    if (seconds <= 0)
        return 0.0f;
    float cpu_usage = 100.0f * ((total_time / (float)clk_tck) / seconds);
    return cpu_usage;
}

std::string GetProcPpid(const char* path) {
    char stat_file[64];
    snprintf(stat_file, sizeof(stat_file), "%s/stat", path);
    FILE* pf = fopen(stat_file, "r");
    if (!pf) return "0";
    
    int pid, ppid;
    char comm[256], state;
    fscanf(pf, "%d %s %c %d", &pid, comm, &state, &ppid);
    fclose(pf);
    return std::to_string(ppid);
}

void ReadMemInfo() {
    // const char* mem_info = "/proc/meminfo";
    FILE *pf = fopen("/proc/meminfo", "r");
    if (pf == nullptr) {
        printf("[ERROR] Failed opening file\n");
        return;
    }
    char buffer[64];
    float memory = 0;
    float memMB = 0;
    while (fgets(buffer, sizeof(buffer), pf)) {
        if (strncmp(buffer, "Active:", 7) == 0) {
            sscanf(buffer + 7, "%f", &memory);
            memMB = memory / 1024.0f;
            break;
        }
    }
    // ImGui::Text("[INFO] Total Memory by kb: %.2f", memory);
    // ImGui::Text("[INFO] Total Memory: %.2fMB", memMB);
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f),
                       "[INFO] Total Memory: %.2fMB", memMB);
    fclose(pf);
}

int GetMemoryUsage(const char *path) {
    char status_file[64];
    snprintf(status_file, sizeof(status_file), "%s/status", path);

    FILE *pf = fopen(status_file, "r");
    if (pf == nullptr) {
        fprintf(stderr, "[ERROR] Failed opening file\n");
        return -1;
    }

    char line[64];
    int memory = -1;

    while (fgets(line, sizeof(line), pf)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line + 6, "%d", &memory); // Skip "VmRSS:"
            // printf("[INFO] %s", line);
            break;
        }
    }
    fclose(pf);
    return memory;
}

void KillProc(std::string proc_pid) {
    pid_t pid;
    try {
        pid_t pid = std::stoi(proc_pid);
        if (kill(pid, SIGKILL) != 0) {
            std::cout << "[ERROR] Failed Killing Task\n";
        }
    } catch (const std::invalid_argument &e) {
        std::cerr << "Invalid number\n";
    } catch (const std::out_of_range &e) {
        std::cerr << "Number out of range\n";
    }
}

void ShowProcessesTree() {
    ImGui::BeginChild("ProcTree", ImVec2(0, 400), true);

    static ImGuiTableFlags flags =
        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("ProcTreeTable", 7, flags)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("User", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("%CPU", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Mem (MB)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Threads", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Command");
        ImGui::TableHeadersRow();

        // Show only root processes (PPID=0 or not found)
        for (int i = 0; i < (int)Procs.size(); i++) {
            if (Procs[i].ParentId == "0") {
                ShowProcessNode(i); // recursive renderer
            }
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();
}

void ShowProcessNode(int idx) {
    Process &proc = Procs[idx];

    ImGui::TableNextRow();
    ImGui::PushID(idx);

    // Color logic
    ImVec4 cpu_color =
        (proc.CpuUsage > 50.0f)   ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f)  // red
        : (proc.CpuUsage > 20.0f) ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f)  // yellow
        : ImVec4(0.3f, 1.0f, 0.3f, 1.0f);                           // green

    float memMB = proc.MemUsage / 1024.0f;
    ImVec4 mem_color =
        (memMB > 200.0f)   ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f)  // red
        : (memMB > 100.0f) ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f)  // yellow
        : ImVec4(0.3f, 1.0f, 0.3f, 1.0f);                   // green

    // subtle color by depth in tree (for hierarchy clarity)
    int depth = 0;
    for (int parent = std::stoi(proc.ParentId);
         parent != 0 && parent < (int)Procs.size(); ) {
        parent = std::stoi(Procs[parent].ParentId);
        depth++;
    }
    float intensity = std::max(0.2f, 1.0f - depth * 0.1f);
    ImVec4 row_tint = ImVec4(intensity, intensity, 1.0f, 1.0f);

    ImGui::TableNextColumn();
    ImGuiTreeNodeFlags nodeFlags =
        ImGuiTreeNodeFlags_SpanFullWidth |
        (proc.Children.empty() ? ImGuiTreeNodeFlags_Leaf : 0) |
        ((proc.Pid == selected_pid) ? ImGuiTreeNodeFlags_Selected : 0);

    // Use tinted color for text (PID)
    ImGui::PushStyleColor(ImGuiCol_Text, row_tint);
    bool open = ImGui::TreeNodeEx(proc.Pid.c_str(), nodeFlags, "%s", proc.Pid.c_str());
    ImGui::PopStyleColor();

    if (ImGui::IsItemClicked()) {
        selected_pid = (proc.Pid == selected_pid ? "" : proc.Pid);
    }

    // Other columns 
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(proc.User.c_str());

    ImGui::TableNextColumn();
    ImGui::TextUnformatted(proc.Name.c_str());

    ImGui::TableNextColumn();
    ImGui::TextColored(cpu_color, "%.1f", proc.CpuUsage);

    ImGui::TableNextColumn();
    ImGui::TextColored(mem_color, "%.1f", memMB);

    ImGui::TableNextColumn();
    ImGui::Text("%d", proc.ThreadCount);

    ImGui::TableNextColumn();
    ImGui::TextUnformatted(proc.Command.c_str());

    // Recursive children rendering
    if (open) {
        for (int childIdx : proc.Children) {
            ShowProcessNode(childIdx);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void CleanupPinned() {
    std::unordered_set<std::string> valid;
    for (const auto &p : Procs)
        valid.insert(p.Pid);

    for (auto it = pinned_pids.begin(); it != pinned_pids.end();)
        if (!valid.count(*it))
            it = pinned_pids.erase(it);
        else
            ++it;
}
