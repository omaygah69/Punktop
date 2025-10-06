#include "json.hpp"
#include "system.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

// For convenience
using json = nlohmann::json;

void WriteSystemJson() {
  using namespace std::literals::chrono_literals;

  while (true) {
    // Create a JSON object
    json root;

    // System
    root["host_name"] = System::s_HostName;
    root["os"] = System::s_OS;
    root["kernel"] = System::s_Kernel;
    root["uptime"] = System::s_Uptime;
    root["shell"] = System::s_Shell;
    root["desktop"] = System::s_Desktop;
    root["window_manager"] = System::s_WindowManager;

    // Hardware
    root["architecture"] = System::s_Arch;
    root["cpu_freq"] = System::s_CpuFreq;

    // CPU
    root["cpu_model"] = System::s_CpuModel;
    root["cpu_usage"] = System::s_CpuUsage;

    // Core
    root["cpu_core"] = System::s_CpuCore; // Automatically converts
                                          // std::vector<float> to JSON array

    // RAM
    root["ram_total"] = System::s_RamTotal;
    root["ram_used"] = System::s_RamUsed;
    root["ram_free"] = System::s_RamFree;

    // Disk
    root["root_disk"] = System::s_RootDisk;
    root["swap_disk"] = System::s_SwapDisk;

    // Write to file with pretty printing (indentation of 4 spaces)
    std::ofstream o("system.json");
    if (!o.is_open()) {
      std::cerr << "Failed to open system.json for writing" << std::endl;
    } else {
      o << root.dump(4) << std::endl;
      o.close();
    }

    // Sleep for 1 second
    std::this_thread::sleep_for(1s);
  }
}
