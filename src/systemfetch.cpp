#include "../include/implot/implot.h"
#include "../punktop.h"
#include <filesystem>
#include <fstream>
#include <ifaddrs.h>
#include <netdb.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <unistd.h>

std::string FormatUptime() {
  std::ifstream f("/proc/uptime");
  double up_seconds = 0.0;
  f >> up_seconds;

  int days = up_seconds / 86400;
  int hours = ((int)up_seconds % 86400) / 3600;
  int minutes = ((int)up_seconds % 3600) / 60;

  std::ostringstream ss;
  if (days > 0)
    ss << days << "d ";
  if (hours > 0)
    ss << hours << "h ";
  ss << minutes << "m";
  return ss.str();
}

std::string GetOSDistribution() {
  std::ifstream f("/etc/os-release");
  std::string line, distro = "Unknown";
  while (std::getline(f, line)) {
    if (line.find("PRETTY_NAME=") != std::string::npos) {
      distro = line.substr(line.find("=") + 1);
      distro.pop_back();  // Remove trailing quote
      distro.erase(0, 1); // Remove leading quote
      break;
    }
  }
  return distro;
}

std::string GetShell() {
  char *shell = getenv("SHELL");
  if (shell) {
    std::string shell_path(shell);
    size_t last_slash = shell_path.find_last_of("/");
    if (last_slash != std::string::npos) {
      return shell_path.substr(last_slash + 1);
    }
    return shell_path;
  }
  return "Unknown";
}

std::string GetDesktopEnvironment() {
  char *de = getenv("XDG_CURRENT_DESKTOP");
  if (de) {
    return std::string(de);
  }
  return "Unknown";
}

std::string GetWindowManager() {
  char *wm = getenv("WINDOWMANAGER");
  if (wm) {
    return std::string(wm);
  }
  if (getenv("WAYLAND_DISPLAY")) {
    return "Wayland";
  }
  std::string wm_names[] = {"i3", "openbox", "bspwm", "dwm", "awesome"};
  for (const auto &name : wm_names) {
    std::string path = "/proc";
    for (const auto &entry : std::filesystem::directory_iterator(path)) {
      if (entry.is_directory()) {
        std::string cmdline_path = entry.path().string() + "/comm";
        std::ifstream f(cmdline_path);
        std::string comm;
        std::getline(f, comm);
        if (comm == name) {
          return name;
        }
      }
    }
  }
  return "Unknown";
}

std::string GetLocalIP() {
  struct ifaddrs *ifaddr, *ifa;
  char host[NI_MAXHOST];
  std::string ip = "Unknown";

  if (getifaddrs(&ifaddr) == -1) {
    return ip;
  }

  for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET) {
      continue;
    }
    if (std::string(ifa->ifa_name) == "lo") {
      continue;
    }
    if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST,
                    nullptr, 0, NI_NUMERICHOST) == 0) {
      ip = host;
      break;
    }
  }
  freeifaddrs(ifaddr);
  return ip;
}

std::string GetCPUFrequency() {
  std::ifstream f("/proc/cpuinfo");
  std::string line, freq = "Unknown";
  while (std::getline(f, line)) {
    if (line.find("cpu MHz") != std::string::npos) {
      freq = line.substr(line.find(":") + 2);
      freq += " MHz";
      break;
    }
  }
  return freq;
}

std::string GetBatteryStatus() {
  std::string path = "/sys/class/power_supply/BAT0";
  if (!std::filesystem::exists(path)) {
    return "No Battery";
  }

  std::ifstream status_file(path + "/status");
  std::ifstream capacity_file(path + "/capacity");
  std::string status;
  int capacity = 0;

  if (status_file.is_open()) {
    std::getline(status_file, status);
  } else {
    status = "Unknown";
  }

  if (capacity_file.is_open()) {
    capacity_file >> capacity;
  }

  std::ostringstream ss;
  ss << status << " (" << capacity << "%)";
  return ss.str();
}

std::string GetSwapUsage() {
  std::ifstream f("/proc/meminfo");
  std::string key;
  long value;
  std::string unit;
  long swap_total = 0, swap_free = 0;

  while (f >> key >> value >> unit) {
    if (key == "SwapTotal:")
      swap_total = value;
    else if (key == "SwapFree:")
      swap_free = value;
  }

  float used_mb = (swap_total - swap_free) / 1024.0f;
  float total_mb = swap_total / 1024.0f;
  float usage_percent =
      (total_mb > 0.0f) ? (used_mb / total_mb) * 100.0f : 0.0f;

  std::ostringstream ss;
  ss << usage_percent << "% (" << used_mb << " MB / " << total_mb << " MB)";
  return ss.str();
}

SystemInfo ReadSystemInfo() {
  SystemInfo info;

  // Hostname
  char host[256];
  gethostname(host, sizeof(host));
  info.hostname = host;

  // Kernel and Architecture
  struct utsname uts;
  uname(&uts);
  info.kernel = std::string(uts.sysname) + " " + uts.release;
  info.arch = uts.machine;

  // Uptime
  info.uptime = FormatUptime();

  // OS Distribution
  info.os_distro = GetOSDistribution();

  // Shell
  info.shell = GetShell();

  // Desktop Environment
  info.desktop_env = GetDesktopEnvironment();

  // Window Manager
  info.window_manager = GetWindowManager();

  // Root Filesystem Usage
  struct statvfs vfs;
  if (statvfs("/", &vfs) == 0) {
    unsigned long long total = vfs.f_blocks * vfs.f_frsize;
    unsigned long long free = vfs.f_bfree * vfs.f_frsize;
    unsigned long long used = total - free;
    info.root_total_gb = total / (1024.0f * 1024.0f * 1024.0f);
    info.root_usage_percent =
        (info.root_total_gb > 0.0f)
            ? (used / (1024.0f * 1024.0f * 1024.0f) / info.root_total_gb) *
                  100.0f
            : 0.0f;
  } else {
    info.root_total_gb = 0.0f;
    info.root_usage_percent = 0.0f;
  }

  // CPU model
  {
    std::ifstream f("/proc/cpuinfo");
    std::string line;
    while (std::getline(f, line)) {
      if (line.find("model name") != std::string::npos) {
        info.cpu_model = line.substr(line.find(":") + 2);
        break;
      }
    }
  }

  // CPU cores
  info.cores = sysconf(_SC_NPROCESSORS_ONLN);

  // CPU frequency
  info.cpu_freq = GetCPUFrequency();

  // RAM size
  {
    std::ifstream f("/proc/meminfo");
    std::string key;
    long mem_kb;
    std::string unit;
    while (f >> key >> mem_kb >> unit) {
      if (key == "MemTotal:") {
        info.ram_total_gb = mem_kb / 1024.0f / 1024.0f;
        break;
      }
    }
  }

  // Swap usage
  info.swap_usage = GetSwapUsage();

  // Battery status
  info.battery_status = GetBatteryStatus();

  // GPU model
  {
    std::ifstream f("/proc/driver/nvidia/gpus/0/information");
    if (f) {
      std::string line;
      while (std::getline(f, line)) {
        if (line.find("Model") != std::string::npos) {
          info.gpu_model = line.substr(line.find(":") + 2);
          break;
        }
      }
    } else {
      for (const auto &entry :
           std::filesystem::directory_iterator("/sys/class/drm")) {
        std::string path = entry.path();
        if (path.find("card") != std::string::npos &&
            std::filesystem::exists(path + "/device/vendor")) {

          std::ifstream vendor_file(path + "/device/vendor");
          std::string vendor;
          vendor_file >> vendor;

          if (vendor == "0x1002") {
            std::ifstream name_file(path + "/device/name");
            if (name_file) {
              std::getline(name_file, info.gpu_model);
              break;
            }
          }
        }
      }
    }
    if (info.gpu_model.empty())
      info.gpu_model = "Unknown GPU";
  }

  return info;
}

void ShowSystemInfo(const SystemInfo &info) {
  ImGui::BeginChild("SystemInfoPanel", ImVec2(0, 0), true);

  ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "System Information");
  ImGui::Separator();

  // System Section
  ImGui::Text("Hostname:   %s", info.hostname.c_str());
  ImGui::Text("OS:         %s", info.os_distro.c_str());
  ImGui::Text("Kernel:     %s", info.kernel.c_str());
  ImGui::Text("Uptime:     %s", info.uptime.c_str());
  ImGui::Separator();

  // Session Section
  ImGui::Text("Shell:      %s", info.shell.c_str());
  ImGui::Text("Desktop:    %s", info.desktop_env.c_str());
  ImGui::Text("WM:         %s", info.window_manager.c_str());
  ImGui::Separator();

  // Hardware Section
  ImGui::Text("Arch:       %s", info.arch.c_str());
  ImGui::Text("CPU:        %s", info.cpu_model.c_str());
  ImGui::Text("CPU Freq:   %s", info.cpu_freq.c_str());
  ImGui::Text("Cores:      %d", info.cores);
  ImGui::Text("RAM:        %.1f GB", info.ram_total_gb);
  ImGui::Text("Swap:       %s", info.swap_usage.c_str());
  ImGui::Text("Root Disk:  %.1f%% / %.1f GB", info.root_usage_percent,
              info.root_total_gb);
  ImGui::Text("GPU:        %s", info.gpu_model.c_str());
  ImGui::Text("Battery:    %s", info.battery_status.c_str());

  ImGui::EndChild();
}
