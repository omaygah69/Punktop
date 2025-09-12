#include "../punktop.h"
#include "../include/implot/implot.h"
#include <sys/utsname.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <filesystem>

std::string FormatUptime() {
    std::ifstream f("/proc/uptime");
    double up_seconds = 0.0;
    f >> up_seconds;

    int days    = up_seconds / 86400;
    int hours   = ((int)up_seconds % 86400) / 3600;
    int minutes = ((int)up_seconds % 3600) / 60;

    std::ostringstream ss;
    if (days > 0) ss << days << "d ";
    if (hours > 0) ss << hours << "h ";
    ss << minutes << "m";
    return ss.str();
}

SystemInfo ReadSystemInfo() {
    SystemInfo info;

    // Hostname
    char host[256];
    gethostname(host, sizeof(host));
    info.hostname = host;

    // Kernel
    struct utsname uts;
    uname(&uts);
    info.kernel = std::string(uts.sysname) + " " + uts.release;

    // Uptime
    info.uptime = FormatUptime();

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

    // GPU model (basic: NVIDIA or AMD)
    // NVIDIA: NVML; AMD: read /sys/class/drm
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
            // Try AMD sysfs
            for (const auto& entry : std::filesystem::directory_iterator("/sys/class/drm")) {
                std::string path = entry.path();
                if (path.find("card") != std::string::npos &&
                    std::filesystem::exists(path + "/device/vendor")) {

                    std::ifstream vendor_file(path + "/device/vendor");
                    std::string vendor;
                    vendor_file >> vendor;

                    if (vendor == "0x1002") { // AMD vendor ID
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

void ShowSystemInfo(const SystemInfo& info) {
    ImGui::BeginChild("SystemInfoPanel", ImVec2(0, 0), true);

    ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "System Information");
    ImGui::Separator();

    ImGui::Text("Hostname:   %s", info.hostname.c_str());
    ImGui::Text("Kernel:     %s", info.kernel.c_str());
    ImGui::Text("Uptime:     %s", info.uptime.c_str());
    ImGui::Separator();

    ImGui::Text("CPU:        %s", info.cpu_model.c_str());
    ImGui::Text("Cores:      %d", info.cores);
    ImGui::Text("RAM:        %.1f GB", info.ram_total_gb);
    ImGui::Separator();

    ImGui::Text("GPU:        %s", info.gpu_model.c_str());

    ImGui::EndChild();
}
