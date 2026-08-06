#include "system.h"
#include "json.hpp"
#include <thread>
#include <fstream>

using json = nlohmann::json;

void WriteSystemJson() {
    using namespace std::literals::chrono_literals;

    while (true) {
        json root;

        // system
        root["host_name"]      = System::s_HostName;
        root["os"]             = System::s_OS;
        root["kernel"]         = System::s_Kernel;
        root["uptime"]         = System::s_Uptime;
        root["shell"]          = System::s_Shell;
        root["desktop"]        = System::s_Desktop;
        root["window_manager"] = System::s_WindowManager;

        // hardware
        root["architecture"]   = System::s_Arch;
        root["cpu_freq"]       = System::s_CpuFreq;

        // cpu
        root["cpu_model"]      = System::s_CpuModel;
        root["cpu_usage"]      = System::s_CpuUsage;

        // core
        root["cpu_core"]       = System::s_CpuCore;  // vector<float> directly works

        // ram
        root["ram_total"]      = System::s_RamTotal;
        root["ram_used"]       = System::s_RamUsed;
        root["ram_free"]       = System::s_RamFree;

        // disk
        root["root_disk"]      = System::s_RootDisk;
        root["swap_disk"]      = System::s_SwapDisk;

        // write to file
        try {
            std::ofstream file("system.json");
            file << root.dump(4); // pretty print with indent = 4
        } catch (const std::exception &e) {
            fprintf(stderr, "Failed to write system.json: %s\n", e.what());
        }

        std::this_thread::sleep_for(1s);
    }
}
