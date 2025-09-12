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
#include <pwd.h>
#include <sys/stat.h>

#define STAT_FILE_PATH "/proc/stat"
#define MAX_CORES 128
#define SLEEP_INTERVAL 1

extern std::atomic<bool> is_finished;
extern std::string search_query;
// extern bool fetch_finished;
// extern std::mutex mtx;

typedef struct {
    std::string Pid;
    std::string Name;
    std::string User;
    std::string Command;
    float CpuUsage;
    float MemUsage;
    int ThreadCount;
} Process;

enum SortMode
{
    no_sort,
    name_sort,
    name_desc,
    pid_sort,
    pid_desc,
    mem_sort,
    mem_desc,
    cpu_sort,
    cpu_desc,
    thread_sort,
    thread_desc,
};

struct NetStat {
    unsigned long long rx_bytes = 0;
    unsigned long long tx_bytes = 0;
};

struct NetHistory {
    std::vector<float> rx_history;
    std::vector<float> tx_history;
    NetStat last_stat{};
};

struct SystemInfo {
    std::string hostname;
    std::string kernel;
    std::string uptime;
    std::string cpu_model;
    int cores = 0;
    std::string gpu_model;
    float ram_total_gb = 0.0f;
};

// struct NetData {
//     NetStat stat;
//     std::vector<float> rx_history;
//     std::vector<float> tx_history;
// };

typedef struct {
    size_t user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
} Core_t;

extern SortMode sortMode;
Process* CreateProcess(unsigned int pid, char* name, float memusage);
void ShowDockSpace(bool& p_open);
bool IsNumeric(std::string dir_name);
std::string GetProcName(const char* path);
int GetMemoryUsage(const char* path);
std::string GetProcUser(const char* path);
std::string GetProcCommand(const char* path);
float GetProcCpuUsage(const std::string& pid);
int GetProcThreadCount(const char* path);
void ReadMemInfo();
void FetchProcesses();
void ShowProcesses();
void ShowProcessesV();
void GetNetworkUsage(const std::string& iface);
void ShowNetworkUsage();
void GetCpuUsage();
void ShowCpuUsage();
void ShowDiskWindow();
void KillProc(std::string proc_pid);
void SortProcesses(SortMode mode);
void FetchMemoryUsage();
void ShowMemoryUsage();
void FetchDiskUsage();
void ShowDiskUsage();
SystemInfo ReadSystemInfo();
void ShowSystemInfo(const SystemInfo& info);
