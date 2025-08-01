#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include <unistd.h>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>

#define STAT_FILE_PATH "/proc/stat"
#define MAX_CORES 128
#define SLEEP_INTERVAL 1

extern std::atomic<bool> is_finished;
// extern bool fetch_finished;
// extern std::mutex mtx;

typedef struct {
    std::string Pid;
    std::string Name;
    float MemUsage;
} Process;

typedef struct {
    size_t user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
} Core_t;

Process* CreateProcess(unsigned int pid, char* name, float memusage);
void ShowDockSpace(bool& p_open);
bool IsNumeric(std::string dir_name);
char* GetProcName(const char* dir_path);
int GetMemoryUsage(const char* path);
void ReadMemInfo();
void FetchProcesses();
void ShowProcesses();
void GetCpuUsage();
void ShowCpuUsage();
void ShowDiskWindow();
