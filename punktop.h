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

struct NetStat {
    unsigned long long rx_bytes = 0;
    unsigned long long tx_bytes = 0;
};

struct NetData {
    NetStat stat;
    std::vector<float> rx_history;
    std::vector<float> tx_history;
};

typedef struct {
    size_t user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
} Core_t;

Process* CreateProcess(unsigned int pid, char* name, float memusage);
void ShowDockSpace(bool& p_open);
bool IsNumeric(std::string dir_name);
char* GetProcName(const char* path);
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
