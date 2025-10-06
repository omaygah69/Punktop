#include "imgui.h"
#include <algorithm>
#include <arpa/inet.h>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ifaddrs.h>
#include <iomanip>
#include <map>
#include <mutex>
#include <netinet/in.h>
#include <set>
#include <sstream>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace NerdStats {

struct SystemStats {
  float cpu_usage = 0.0f;            // % CPU usage (total)
  std::vector<float> cpu_core_usage; // % CPU usage per core
  std::vector<float> cpu_core_freq;  // MHz per core
  int cpu_cores = 0;                 // Number of CPU cores
  int cpu_threads = 0;               // Number of CPU threads
  float load_avg_1 = 0.0f;           // 1-minute load average
  float load_avg_5 = 0.0f;           // 5-minute load average
  float load_avg_15 = 0.0f;          // 15-minute load average
  std::string cpu_model;             // CPU model name
  std::string cpu_freq;              // Average CPU frequency
  std::string cpu_cache;             // L1, L2, L3 cache sizes
  float ram_total = 0.0f;            // GB
  float ram_used = 0.0f;             // GB
  float ram_free = 0.0f;             // GB
  float ram_buffers = 0.0f;          // GB
  float ram_cached = 0.0f;           // GB
  float ram_active = 0.0f;           // GB
  float ram_inactive = 0.0f;         // GB
  float swap_total = 0.0f;           // GB
  float swap_used = 0.0f;            // GB
  float disk_used = 0.0f;            // GB
  float disk_total = 0.0f;           // GB
  float disk_read_kb = 0.0f;         // KB/s
  float disk_write_kb = 0.0f;        // KB/s
  std::string disk_partitions;       // List of disk partitions and usage
  std::string filesystem_types;      // Filesystem types
  std::map<std::string, std::pair<float, float>>
      disk_io;                   // Per-partition I/O (read KB/s, write KB/s)
  float net_rx_kb = 0.0f;        // KB/s
  float net_tx_kb = 0.0f;        // KB/s
  float net_rx_packets = 0.0f;   // Packets/s
  float net_tx_packets = 0.0f;   // Packets/s
  float net_errors = 0.0f;       // Errors/s
  std::string net_interfaces;    // List of active network interfaces
  std::string net_ip_address;    // IP address of primary interface
  std::string net_status;        // Network status (up/down)
  std::string primary_interface; // Primary network interface
  std::map<std::string, std::tuple<float, float, float, float, float>>
      net_stats;         // Per-interface stats
  int process_count = 0; // Number of running processes
};

struct CpuStat {
  long user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0,
       softirq = 0;
  long total() const {
    return user + nice + system + idle + iowait + irq + softirq;
  }
};

struct MemStat {
  float used_gb = 0.0f;
  float total_gb = 0.0f;
  float free_gb = 0.0f;
  float buffers_gb = 0.0f;
  float cached_gb = 0.0f;
  float active_gb = 0.0f;
  float inactive_gb = 0.0f;
  float swap_used_gb = 0.0f;
  float swap_total_gb = 0.0f;
};

struct DiskStat {
  float used_gb = 0.0f;
  float total_gb = 0.0f;
  float read_kb = 0.0f;
  float write_kb = 0.0f;
};

struct NetStat {
  long long rx_bytes = 0;
  long long tx_bytes = 0;
  long long rx_packets = 0;
  long long tx_packets = 0;
  long long errors = 0;
};

static float s_RefreshInterval = 1.0f;
static std::mutex s_RefreshMutex;
static SystemStats s_SystemInfo;
static std::mutex system_mutex;
static std::atomic<bool> s_IsFinished{false};

void SetRefreshInterval(float interval) {
  std::lock_guard<std::mutex> lock(s_RefreshMutex);
  s_RefreshInterval = std::max(0.5f, std::min(interval, 5.0f));
}

float GetRefreshInterval() {
  std::lock_guard<std::mutex> lock(s_RefreshMutex);
  return s_RefreshInterval;
}

CpuStat ReadCpuStat(const std::string &cpu_id = "cpu") {
  std::ifstream file("/proc/stat");
  CpuStat stat;
  if (!file.is_open())
    return stat;

  std::string line;
  while (std::getline(file, line)) {
    if (line.find(cpu_id + " ") == 0) {
      std::istringstream iss(line);
      std::string cpu_label;
      iss >> cpu_label >> stat.user >> stat.nice >> stat.system >> stat.idle >>
          stat.iowait >> stat.irq >> stat.softirq;
      break;
    }
  }
  return stat;
}

MemStat ReadMemStat() {
  MemStat stat;
  std::ifstream file("/proc/meminfo");
  if (!file.is_open())
    return stat;

  std::string key;
  long value;
  std::string unit;
  long mem_total = 0, mem_free = 0, buffers = 0, cached = 0, active = 0,
       inactive = 0, swap_total = 0, swap_free = 0;

  while (file >> key >> value >> unit) {
    if (key == "MemTotal:")
      mem_total = value;
    else if (key == "MemFree:")
      mem_free = value;
    else if (key == "Buffers:")
      buffers = value;
    else if (key == "Cached:")
      cached = value;
    else if (key == "Active:")
      active = value;
    else if (key == "Inactive:")
      inactive = value;
    else if (key == "SwapTotal:")
      swap_total = value;
    else if (key == "SwapFree:")
      swap_free = value;
  }

  long used = mem_total - (mem_free + buffers + cached);
  stat.total_gb = mem_total / 1024.0f / 1024.0f;
  stat.used_gb = used / 1024.0f / 1024.0f;
  stat.free_gb = mem_free / 1024.0f / 1024.0f;
  stat.buffers_gb = buffers / 1024.0f / 1024.0f;
  stat.cached_gb = cached / 1024.0f / 1024.0f;
  stat.active_gb = active / 1024.0f / 1024.0f;
  stat.inactive_gb = inactive / 1024.0f / 1024.0f;
  stat.swap_total_gb = swap_total / 1024.0f / 1024.0f;
  stat.swap_used_gb = (swap_total - swap_free) / 1024.0f / 1024.0f;
  return stat;
}

DiskStat ReadDiskStat(const char *path = "/") {
  DiskStat stat;
  struct statvfs vfs;
  if (statvfs(path, &vfs) != 0)
    return stat;

  unsigned long long total = (unsigned long long)vfs.f_blocks * vfs.f_frsize;
  unsigned long long free = (unsigned long long)vfs.f_bfree * vfs.f_frsize;
  unsigned long long used = total - free;

  stat.total_gb = total / (1024.0f * 1024.0f * 1024.0f);
  stat.used_gb = used / (1024.0f * 1024.0f * 1024.0f);

  std::ifstream diskstats("/proc/diskstats");
  if (diskstats.is_open()) {
    std::string line;
    while (std::getline(diskstats, line)) {
      if (line.find("nvme0n1 ") != std::string::npos) {
        std::istringstream iss(line);
        std::string temp;
        long long reads, read_sectors, writes, write_sectors;
        iss >> temp >> temp >> temp >> reads >> read_sectors >> temp >> temp >>
            writes >> write_sectors;
        static std::map<std::string, std::pair<long long, long long>> prev_io;
        std::string disk = "nvme0n1";
        if (prev_io.count(disk)) {
          auto [prev_reads, prev_writes] = prev_io[disk];
          stat.read_kb =
              (read_sectors - prev_reads) * 0.5f / GetRefreshInterval();
          stat.write_kb =
              (write_sectors - prev_writes) * 0.5f / GetRefreshInterval();
        }
        prev_io[disk] = {read_sectors, write_sectors};
        break;
      }
    }
  }
  return stat;
}

std::map<std::string, std::pair<float, float>> ReadDiskIO() {
  std::map<std::string, std::pair<float, float>> io_stats;
  std::ifstream diskstats("/proc/diskstats");
  if (!diskstats.is_open())
    return io_stats;

  static std::map<std::string, std::pair<long long, long long>> prev_io;
  std::string line;
  while (std::getline(diskstats, line)) {
    std::istringstream iss(line);
    std::string major, minor, device;
    long long reads, read_sectors, writes, write_sectors;
    iss >> major >> minor >> device >> reads >> read_sectors >> std::ws;
    for (int i = 0; i < 3; ++i)
      iss >> std::ws;
    iss >> writes >> write_sectors;
    if (device.find("nvme") != std::string::npos ||
        device.find("sd") != std::string::npos) {
      if (prev_io.count(device)) {
        auto [prev_reads, prev_writes] = prev_io[device];
        float read_kb =
            (read_sectors - prev_reads) * 0.5f / GetRefreshInterval();
        float write_kb =
            (write_sectors - prev_writes) * 0.5f / GetRefreshInterval();
        io_stats[device] = {read_kb, write_kb};
      }
      prev_io[device] = {read_sectors, write_sectors};
    }
  }
  return io_stats;
}

std::string GetPrimaryInterface() {
  std::ifstream file("/proc/net/dev");
  std::string primary_iface = "Unknown";
  long long max_rx_bytes = 0;
  if (!file.is_open())
    return primary_iface;

  std::string line;
  std::getline(file, line); // Skip header
  std::getline(file, line); // Skip header
  while (std::getline(file, line)) {
    size_t pos = line.find(":");
    if (pos != std::string::npos) {
      std::string iface = line.substr(0, pos);
      iface.erase(std::remove_if(iface.begin(), iface.end(), ::isspace),
                  iface.end());
      if (!iface.empty() && iface != "lo") {
        std::istringstream iss(line.substr(pos + 1));
        long long rx_bytes;
        iss >> rx_bytes;
        if (rx_bytes > max_rx_bytes) {
          max_rx_bytes = rx_bytes;
          primary_iface = iface;
        }
      }
    }
  }
  return primary_iface.empty() ? "None" : primary_iface;
}

NetStat ReadNetStat(const std::string &iface) {
  NetStat stat;
  std::ifstream file("/proc/net/dev");
  if (!file.is_open())
    return stat;

  std::string line;
  while (std::getline(file, line)) {
    if (line.find(iface + ":") != std::string::npos) {
      std::istringstream iss(line.substr(line.find(":") + 1));
      iss >> stat.rx_bytes >> stat.rx_packets;
      for (int i = 0; i < 6; i++)
        iss >> std::ws;
      iss >> stat.tx_bytes >> stat.tx_packets >> stat.errors;
      break;
    }
  }
  return stat;
}

std::string GetNetworkInterfaces() {
  std::string interfaces;
  std::ifstream file("/proc/net/dev");
  if (!file.is_open())
    return "Unknown";

  std::string line;
  std::getline(file, line); // Skip header
  std::getline(file, line); // Skip header
  while (std::getline(file, line)) {
    size_t pos = line.find(":");
    if (pos != std::string::npos) {
      std::string iface = line.substr(0, pos);
      iface.erase(std::remove_if(iface.begin(), iface.end(), ::isspace),
                  iface.end());
      if (!iface.empty() && iface != "lo") {
        interfaces += iface + ", ";
      }
    }
  }
  if (!interfaces.empty()) {
    interfaces.erase(interfaces.length() - 2);
  } else {
    interfaces = "None";
  }
  return interfaces;
}

std::string GetIPAddress(const std::string &iface) {
  std::string ip = "Unknown";
  struct ifaddrs *ifaddr, *ifa;
  if (getifaddrs(&ifaddr) == -1)
    return ip;

  for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET &&
        iface == ifa->ifa_name) {
      char ip_addr[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr,
                ip_addr, INET_ADDRSTRLEN);
      ip = ip_addr;
      break;
    }
  }
  freeifaddrs(ifaddr);
  return ip;
}

std::string GetNetworkStatus(const std::string &iface) {
  std::string path = "/sys/class/net/" + iface + "/operstate";
  std::ifstream file(path);
  std::string status;
  if (file.is_open()) {
    std::getline(file, status);
    return status.empty() ? "Unknown" : status;
  }
  return "Unknown";
}

std::map<std::string, std::tuple<float, float, float, float, float>>
ReadNetStats() {
  std::map<std::string, std::tuple<float, float, float, float, float>> stats;
  std::ifstream file("/proc/net/dev");
  if (!file.is_open())
    return stats;

  static std::map<std::string, NetStat> prev_stats;
  std::string line;
  std::getline(file, line); // Skip header
  std::getline(file, line); // Skip header
  while (std::getline(file, line)) {
    size_t pos = line.find(":");
    if (pos != std::string::npos) {
      std::string iface = line.substr(0, pos);
      iface.erase(std::remove_if(iface.begin(), iface.end(), ::isspace),
                  iface.end());
      if (!iface.empty() && iface != "lo") {
        NetStat curr;
        std::istringstream iss(line.substr(pos + 1));
        iss >> curr.rx_bytes >> curr.rx_packets;
        for (int i = 0; i < 6; i++)
          iss >> std::ws;
        iss >> curr.tx_bytes >> curr.tx_packets >> curr.errors;
        if (prev_stats.count(iface)) {
          auto &prev = prev_stats[iface];
          float rx_kb =
              (curr.rx_bytes - prev.rx_bytes) / 1024.0f / GetRefreshInterval();
          float tx_kb =
              (curr.tx_bytes - prev.tx_bytes) / 1024.0f / GetRefreshInterval();
          float rx_packets =
              (curr.rx_packets - prev.rx_packets) / GetRefreshInterval();
          float tx_packets =
              (curr.tx_packets - prev.tx_packets) / GetRefreshInterval();
          float errors = (curr.errors - prev.errors) / GetRefreshInterval();
          stats[iface] = {rx_kb, tx_kb, rx_packets, tx_packets, errors};
        }
        prev_stats[iface] = curr;
      }
    }
  }
  return stats;
}

SystemStats ReadSystemInfo() {
  SystemStats info;
  // CPU Info
  std::ifstream cpuinfo("/proc/cpuinfo");
  if (cpuinfo.is_open()) {
    std::string line;
    int core_count = 0;
    std::vector<float> core_freqs;
    while (std::getline(cpuinfo, line)) {
      if (line.find("model name") != std::string::npos) {
        info.cpu_model = line.substr(line.find(":") + 2);
      } else if (line.find("cpu MHz") != std::string::npos) {
        try {
          float freq = std::stof(line.substr(line.find(":") + 2));
          core_freqs.push_back(freq);
        } catch (...) {
          // Handle parsing error
        }
      } else if (line.find("cpu cores") != std::string::npos) {
        std::istringstream iss(line);
        std::string key;
        iss >> key >> key >> key >> info.cpu_cores;
      } else if (line.find("siblings") != std::string::npos) {
        std::istringstream iss(line);
        std::string key;
        iss >> key >> key >> info.cpu_threads;
      }
    }
    if (!core_freqs.empty()) {
      float avg_freq = 0.0f;
      for (float f : core_freqs)
        avg_freq += f;
      avg_freq /= core_freqs.size();
      info.cpu_freq = std::to_string(avg_freq) + " MHz";
      info.cpu_core_freq = core_freqs;
    }
  }

  // CPU Cache
  std::string cache_info;
  for (int i = 0; i < 4; ++i) {
    std::string path = "/sys/devices/system/cpu/cpu0/cache/index" +
                       std::to_string(i) + "/size";
    std::ifstream cache_file(path);
    if (cache_file.is_open()) {
      std::string size;
      std::getline(cache_file, size);
      if (!size.empty()) {
        cache_info += "L" + std::to_string(i + 1) + ": " + size + ", ";
      }
    }
  }
  if (!cache_info.empty()) {
    cache_info.erase(cache_info.length() - 2);
    info.cpu_cache = cache_info;
  } else {
    info.cpu_cache = "Unknown";
  }

  // Load Averages
  struct sysinfo sys_info;
  if (sysinfo(&sys_info) == 0) {
    info.load_avg_1 = sys_info.loads[0] / (float)(1 << SI_LOAD_SHIFT);
    info.load_avg_5 = sys_info.loads[1] / (float)(1 << SI_LOAD_SHIFT);
    info.load_avg_15 = sys_info.loads[2] / (float)(1 << SI_LOAD_SHIFT);
  }

  // Process Count
  info.process_count = 0;
  for (const auto &entry : std::filesystem::directory_iterator("/proc")) {
    if (entry.is_directory()) {
      std::string dir_name = entry.path().filename().string();
      if (std::all_of(dir_name.begin(), dir_name.end(), ::isdigit)) {
        info.process_count++;
      }
    }
  }

  // Disk Partitions
  std::ifstream mounts("/proc/mounts");
  if (mounts.is_open()) {
    std::string line;
    while (std::getline(mounts, line)) {
      std::istringstream iss(line);
      std::string device, mount_point, fs_type;
      iss >> device >> mount_point >> fs_type;
      if (device.find("/dev/") == 0) {
        DiskStat part_stat = ReadDiskStat(mount_point.c_str());
        if (part_stat.total_gb > 0) {
          info.disk_partitions += mount_point + " (" + fs_type +
                                  "): " + std::to_string(part_stat.used_gb) +
                                  " GB / " +
                                  std::to_string(part_stat.total_gb) + " GB, ";
        }
      }
    }
    if (!info.disk_partitions.empty()) {
      info.disk_partitions.erase(info.disk_partitions.length() - 2);
    } else {
      info.disk_partitions = "None";
    }
  }

  // Filesystem Types
  std::ifstream mounts2("/proc/mounts");
  std::set<std::string> fs_types;
  if (mounts2.is_open()) {
    std::string line;
    while (std::getline(mounts2, line)) {
      std::istringstream iss(line);
      std::string device, mount_point, fs_type;
      iss >> device >> mount_point >> fs_type;
      if (!fs_type.empty())
        fs_types.insert(fs_type);
    }
    for (const auto &fs : fs_types) {
      info.filesystem_types += fs + ", ";
    }
    if (!info.filesystem_types.empty()) {
      info.filesystem_types.erase(info.filesystem_types.length() - 2);
    } else {
      info.filesystem_types = "None";
    }
  }

  info.net_interfaces = GetNetworkInterfaces();
  info.primary_interface = GetPrimaryInterface();
  info.net_ip_address = GetIPAddress(info.primary_interface);
  info.net_status = GetNetworkStatus(info.primary_interface);

  return info;
}

void FetchCpuUsage() {
  {
    std::lock_guard<std::mutex> lock(system_mutex);
    if (s_SystemInfo.cpu_cores == 0) {
      SystemStats info = ReadSystemInfo();
      s_SystemInfo.cpu_cores = info.cpu_cores;
      s_SystemInfo.cpu_threads = info.cpu_threads;
    }
  }
  CpuStat prev_stat = ReadCpuStat();
  std::vector<CpuStat> prev_core_stats(s_SystemInfo.cpu_cores);
  for (int i = 0; i < s_SystemInfo.cpu_cores; ++i) {
    prev_core_stats[i] = ReadCpuStat("cpu" + std::to_string(i));
  }
  while (!s_IsFinished) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds((int)(GetRefreshInterval() * 1000)));
    CpuStat curr_stat = ReadCpuStat();
    std::vector<float> core_usage(s_SystemInfo.cpu_cores);
    for (int i = 0; i < s_SystemInfo.cpu_cores; ++i) {
      CpuStat curr_core = ReadCpuStat("cpu" + std::to_string(i));
      long total_diff = curr_core.total() - prev_core_stats[i].total();
      long idle_diff = curr_core.idle - prev_core_stats[i].idle;
      core_usage[i] = (total_diff > 0)
                          ? 100.0f * (total_diff - idle_diff) / total_diff
                          : 0.0f;
      prev_core_stats[i] = curr_core;
    }

    long prev_total = prev_stat.total();
    long curr_total = curr_stat.total();
    long total_diff = curr_total - prev_total;
    long idle_diff = curr_stat.idle - prev_stat.idle;
    float usage_percent = (total_diff > 0)
                              ? 100.0f * (total_diff - idle_diff) / total_diff
                              : 0.0f;

    {
      std::lock_guard<std::mutex> lock(system_mutex);
      s_SystemInfo.cpu_usage = usage_percent;
      s_SystemInfo.cpu_core_usage = core_usage;
    }

    prev_stat = curr_stat;
  }
}

void FetchMemUsage() {
  while (!s_IsFinished) {
    MemStat stat = ReadMemStat();
    {
      std::lock_guard<std::mutex> lock(system_mutex);
      s_SystemInfo.ram_total = stat.total_gb;
      s_SystemInfo.ram_used = stat.used_gb;
      s_SystemInfo.ram_free = stat.free_gb;
      s_SystemInfo.ram_buffers = stat.buffers_gb;
      s_SystemInfo.ram_cached = stat.cached_gb;
      s_SystemInfo.ram_active = stat.active_gb;
      s_SystemInfo.ram_inactive = stat.inactive_gb;
      s_SystemInfo.swap_total = stat.swap_total_gb;
      s_SystemInfo.swap_used = stat.swap_used_gb;
    }
    std::this_thread::sleep_for(
        std::chrono::milliseconds((int)(GetRefreshInterval() * 1000)));
  }
}

void FetchDiskUsage() {
  while (!s_IsFinished) {
    DiskStat stat = ReadDiskStat();
    auto io_stats = ReadDiskIO();
    {
      std::lock_guard<std::mutex> lock(system_mutex);
      s_SystemInfo.disk_total = stat.total_gb;
      s_SystemInfo.disk_used = stat.used_gb;
      s_SystemInfo.disk_read_kb = stat.read_kb;
      s_SystemInfo.disk_write_kb = stat.write_kb;
      s_SystemInfo.disk_io = io_stats;
    }
    std::this_thread::sleep_for(
        std::chrono::milliseconds((int)(GetRefreshInterval() * 1000)));
  }
}

void FetchNetUsage() {
  std::string iface = GetPrimaryInterface();
  NetStat prev = ReadNetStat(iface);
  while (!s_IsFinished) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds((int)(GetRefreshInterval() * 1000)));
    NetStat curr = ReadNetStat(iface);
    auto net_stats = ReadNetStats();

    float rx_kb =
        (curr.rx_bytes - prev.rx_bytes) / 1024.0f / GetRefreshInterval();
    float tx_kb =
        (curr.tx_bytes - prev.tx_bytes) / 1024.0f / GetRefreshInterval();
    float rx_packets =
        (curr.rx_packets - prev.rx_packets) / GetRefreshInterval();
    float tx_packets =
        (curr.tx_packets - prev.tx_packets) / GetRefreshInterval();
    float errors = (curr.errors - prev.errors) / GetRefreshInterval();

    {
      std::lock_guard<std::mutex> lock(system_mutex);
      s_SystemInfo.net_rx_kb = rx_kb;
      s_SystemInfo.net_tx_kb = tx_kb;
      s_SystemInfo.net_rx_packets = rx_packets;
      s_SystemInfo.net_tx_packets = tx_packets;
      s_SystemInfo.net_errors = errors;
      s_SystemInfo.primary_interface = iface;
      s_SystemInfo.net_ip_address = GetIPAddress(iface);
      s_SystemInfo.net_status = GetNetworkStatus(iface);
      s_SystemInfo.net_stats = net_stats;
    }

    prev = curr;
    iface = GetPrimaryInterface();
  }
}

void FetchSystemInfo() {
  while (!s_IsFinished) {
    SystemStats info = ReadSystemInfo();
    {
      std::lock_guard<std::mutex> lock(system_mutex);
      s_SystemInfo.cpu_model = info.cpu_model;
      s_SystemInfo.cpu_freq = info.cpu_freq;
      s_SystemInfo.cpu_cores = info.cpu_cores;
      s_SystemInfo.cpu_threads = info.cpu_threads;
      s_SystemInfo.cpu_core_freq = info.cpu_core_freq;
      s_SystemInfo.cpu_cache = info.cpu_cache;
      s_SystemInfo.load_avg_1 = info.load_avg_1;
      s_SystemInfo.load_avg_5 = info.load_avg_5;
      s_SystemInfo.load_avg_15 = info.load_avg_15;
      s_SystemInfo.process_count = info.process_count;
      s_SystemInfo.disk_partitions = info.disk_partitions;
      s_SystemInfo.filesystem_types = info.filesystem_types;
      s_SystemInfo.net_interfaces = info.net_interfaces;
      s_SystemInfo.primary_interface = info.primary_interface;
      s_SystemInfo.net_ip_address = info.net_ip_address;
      s_SystemInfo.net_status = info.net_status;
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }
}

void ShowCpuDetails(const SystemStats &info) {
  ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "CPU Information");
  ImGui::Separator();
  ImGui::Text("Model: %s",
              info.cpu_model.empty() ? "Unknown" : info.cpu_model.c_str());
  ImGui::Text("Average Frequency: %s",
              info.cpu_freq.empty() ? "Unknown" : info.cpu_freq.c_str());
  ImGui::Text("Cores: %d", info.cpu_cores);
  ImGui::Text("Threads: %d", info.cpu_threads);
  ImGui::Text("Cache: %s", info.cpu_cache.c_str());
  ImGui::Text("Total Usage: %.1f%%", info.cpu_usage);
  ImGui::Text("Load Average (1/5/15 min): %.2f / %.2f / %.2f", info.load_avg_1,
              info.load_avg_5, info.load_avg_15);
  if (info.cpu_core_usage.size() == info.cpu_cores &&
      !info.cpu_core_usage.empty()) {
    ImGui::Text("Per-Core Usage:");
    for (size_t i = 0; i < info.cpu_core_usage.size(); ++i) {
      float freq =
          (i < info.cpu_core_freq.size()) ? info.cpu_core_freq[i] : 0.0f;
      ImGui::Text("  Core %zu: %.1f%% @ %.1f MHz", i, info.cpu_core_usage[i],
                  freq);
    }
  }
}

void ShowMemoryDetails(const SystemStats &info) {
  ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "Memory Information");
  ImGui::Separator();
  ImGui::Text("Total: %.1f GB", info.ram_total);
  ImGui::Text("Used: %.1f GB (%.1f%%)", info.ram_used,
              info.ram_total > 0 ? (info.ram_used / info.ram_total) * 100.0f
                                 : 0.0f);
  ImGui::Text("Free: %.1f GB", info.ram_free);
  ImGui::Text("Buffers: %.1f GB", info.ram_buffers);
  ImGui::Text("Cached: %.1f GB", info.ram_cached);
  ImGui::Text("Active: %.1f GB", info.ram_active);
  ImGui::Text("Inactive: %.1f GB", info.ram_inactive);
  ImGui::Text("Swap Total: %.1f GB", info.swap_total);
  ImGui::Text("Swap Used: %.1f GB (%.1f%%)", info.swap_used,
              info.swap_total > 0 ? (info.swap_used / info.swap_total) * 100.0f
                                  : 0.0f);
}

void ShowStorageDetails(const SystemStats &info) {
  ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "Storage Information");
  ImGui::Separator();
  ImGui::Text(
      "Root Disk: %.1f GB / %.1f GB (%.1f%%)", info.disk_used, info.disk_total,
      info.disk_total > 0 ? (info.disk_used / info.disk_total) * 100.0f : 0.0f);
  ImGui::Text("Disk I/O: Read %.1f KB/s, Write %.1f KB/s", info.disk_read_kb,
              info.disk_write_kb);
  ImGui::Text("Partitions: %s", info.disk_partitions.empty()
                                    ? "None"
                                    : info.disk_partitions.c_str());
  ImGui::Text("Filesystem Types: %s", info.filesystem_types.empty()
                                          ? "None"
                                          : info.filesystem_types.c_str());
  if (!info.disk_io.empty()) {
    ImGui::Text("Per-Device I/O:");
    for (const auto &[device, io] : info.disk_io) {
      ImGui::Text("  %s: Read %.1f KB/s, Write %.1f KB/s", device.c_str(),
                  io.first, io.second);
    }
  }
}

void ShowNetworkDetails(const SystemStats &info) {
  ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Network Information");
  ImGui::Separator();
  ImGui::Text("Primary Interface: %s", info.primary_interface.c_str());
  ImGui::Text("IP Address: %s", info.net_ip_address.c_str());
  ImGui::Text("Status: %s", info.net_status.c_str());
  ImGui::Text("Interfaces: %s", info.net_interfaces.empty()
                                    ? "None"
                                    : info.net_interfaces.c_str());
  ImGui::Text("Primary Interface Stats:");
  ImGui::Text("  RX: %.1f KB/s, TX: %.1f KB/s", info.net_rx_kb, info.net_tx_kb);
  ImGui::Text("  Packets RX: %.1f/s, TX: %.1f/s", info.net_rx_packets,
              info.net_tx_packets);
  ImGui::Text("  Errors: %.1f/s", info.net_errors);
  if (!info.net_stats.empty()) {
    ImGui::Text("Per-Interface Stats:");
    for (const auto &[iface, stats] : info.net_stats) {
      auto [rx_kb, tx_kb, rx_packets, tx_packets, errors] = stats;
      ImGui::Text("  %s: RX %.1f KB/s, TX %.1f KB/s, Packets RX %.1f/s, TX "
                  "%.1f/s, Errors %.1f/s",
                  iface.c_str(), rx_kb, tx_kb, rx_packets, tx_packets, errors);
    }
  }
}

void ShowResourceUsage(const SystemStats &info) {
  ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Resource Usage");
  ImGui::Separator();
  ImGui::Text("Running Processes: %d", info.process_count);
}

void ShowNerdStats() {
  ImGui::Begin("Stats for Nerds");
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

  ImVec2 region = ImGui::GetContentRegionAvail();
  ImGui::BeginChild("NerdStatsContent", ImVec2(region.x, region.y), true);

  // CPU Information
  {
    std::lock_guard<std::mutex> lock(system_mutex);
    ShowCpuDetails(s_SystemInfo);
  }

  ImGui::Separator();
  ImGui::Spacing();

  // Memory Information
  {
    std::lock_guard<std::mutex> lock(system_mutex);
    ShowMemoryDetails(s_SystemInfo);
  }

  ImGui::Separator();
  ImGui::Spacing();

  // Storage Information
  {
    std::lock_guard<std::mutex> lock(system_mutex);
    ShowStorageDetails(s_SystemInfo);
  }

  ImGui::Separator();
  ImGui::Spacing();

  // Network Information
  {
    std::lock_guard<std::mutex> lock(system_mutex);
    ShowNetworkDetails(s_SystemInfo);
  }

  ImGui::Separator();
  ImGui::Spacing();

  // Resource Usage
  {
    std::lock_guard<std::mutex> lock(system_mutex);
    ShowResourceUsage(s_SystemInfo);
  }

  ImGui::Separator();
  ImGui::Spacing();

  // Developer Tools
  ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Developer Tools");
  ImGui::Separator();
  if (ImGui::Button("Force Refresh")) {
    std::lock_guard<std::mutex> lock(system_mutex);
    CpuStat cpu_stat = ReadCpuStat();
    s_SystemInfo.cpu_usage =
        (cpu_stat.total() > 0)
            ? 100.0f * (cpu_stat.total() - cpu_stat.idle) / cpu_stat.total()
            : 0.0f;
    std::vector<float> core_usage(s_SystemInfo.cpu_cores);
    for (int i = 0; i < s_SystemInfo.cpu_cores; ++i) {
      CpuStat core_stat = ReadCpuStat("cpu" + std::to_string(i));
      core_usage[i] = (core_stat.total() > 0)
                          ? 100.0f * (core_stat.total() - core_stat.idle) /
                                core_stat.total()
                          : 0.0f;
    }
    s_SystemInfo.cpu_core_usage = core_usage;
    MemStat mem_stat = ReadMemStat();
    s_SystemInfo.ram_total = mem_stat.total_gb;
    s_SystemInfo.ram_used = mem_stat.used_gb;
    s_SystemInfo.ram_free = mem_stat.free_gb;
    s_SystemInfo.ram_buffers = mem_stat.buffers_gb;
    s_SystemInfo.ram_cached = mem_stat.cached_gb;
    s_SystemInfo.ram_active = mem_stat.active_gb;
    s_SystemInfo.ram_inactive = mem_stat.inactive_gb;
    s_SystemInfo.swap_total = mem_stat.swap_total_gb;
    s_SystemInfo.swap_used = mem_stat.swap_used_gb;
    DiskStat disk_stat = ReadDiskStat();
    s_SystemInfo.disk_total = disk_stat.total_gb;
    s_SystemInfo.disk_used = disk_stat.used_gb;
    s_SystemInfo.disk_read_kb = disk_stat.read_kb;
    s_SystemInfo.disk_write_kb = disk_stat.write_kb;
    s_SystemInfo.disk_io = ReadDiskIO();
    NetStat net_stat = ReadNetStat(s_SystemInfo.primary_interface);
    s_SystemInfo.net_rx_kb = net_stat.rx_bytes / 1024.0f / GetRefreshInterval();
    s_SystemInfo.net_tx_kb = net_stat.tx_bytes / 1024.0f / GetRefreshInterval();
    s_SystemInfo.net_rx_packets = net_stat.rx_packets / GetRefreshInterval();
    s_SystemInfo.net_tx_packets = net_stat.tx_packets / GetRefreshInterval();
    s_SystemInfo.net_errors = net_stat.errors / GetRefreshInterval();
    s_SystemInfo.net_stats = ReadNetStats();
    SystemStats sys_info = ReadSystemInfo();
    s_SystemInfo.cpu_model = sys_info.cpu_model;
    s_SystemInfo.cpu_freq = sys_info.cpu_freq;
    s_SystemInfo.cpu_cores = sys_info.cpu_cores;
    s_SystemInfo.cpu_threads = sys_info.cpu_threads;
    s_SystemInfo.cpu_core_freq = sys_info.cpu_core_freq;
    s_SystemInfo.cpu_cache = sys_info.cpu_cache;
    s_SystemInfo.load_avg_1 = sys_info.load_avg_1;
    s_SystemInfo.load_avg_5 = sys_info.load_avg_5;
    s_SystemInfo.load_avg_15 = sys_info.load_avg_15;
    s_SystemInfo.process_count = sys_info.process_count;
    s_SystemInfo.disk_partitions = sys_info.disk_partitions;
    s_SystemInfo.filesystem_types = sys_info.filesystem_types;
    s_SystemInfo.net_interfaces = sys_info.net_interfaces;
    s_SystemInfo.primary_interface = sys_info.primary_interface;
    s_SystemInfo.net_ip_address = sys_info.net_ip_address;
    s_SystemInfo.net_status = sys_info.net_status;
  }
  ImGui::SameLine();
  ImGui::SetNextItemWidth(100.0f);
  ImGui::SliderFloat("Refresh (s)", &s_RefreshInterval, 0.5f, 5.0f);

  ImGui::EndChild();
  ImGui::PopStyleColor();
  ImGui::End();
}

void StartStatsForNerdsThreads() {
  // Initialize SystemStats to avoid uninitialized cpu_cores
  {
    std::lock_guard<std::mutex> lock(system_mutex);
    SystemStats info = ReadSystemInfo();
    s_SystemInfo.cpu_cores = info.cpu_cores;
    s_SystemInfo.cpu_threads = info.cpu_threads;
    s_SystemInfo.cpu_model = info.cpu_model;
    s_SystemInfo.cpu_freq = info.cpu_freq;
    s_SystemInfo.cpu_core_freq = info.cpu_core_freq;
    s_SystemInfo.cpu_cache = info.cpu_cache;
  }

  std::thread cpu_thread(FetchCpuUsage);
  cpu_thread.detach();
  std::thread mem_thread(FetchMemUsage);
  mem_thread.detach();
  std::thread disk_thread(FetchDiskUsage);
  disk_thread.detach();
  std::thread net_thread(FetchNetUsage);
  net_thread.detach();
  std::thread sys_thread(FetchSystemInfo);
  sys_thread.detach();
}
} // namespace NerdStats
