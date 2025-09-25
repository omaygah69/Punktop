#include "system.h"
#include <jansson.h>
#include <thread>

void WriteSystemJson() {
  using namespace std::literals::chrono_literals;

  while (true) {
    json_t *root = json_object();

    // system
    json_object_set_new(root, "host_name",
                        json_string(System::s_HostName.c_str()));
    json_object_set_new(root, "os", json_string(System::s_OS.c_str()));
    json_object_set_new(root, "kernel", json_string(System::s_Kernel.c_str()));
    json_object_set_new(root, "uptime", json_string(System::s_Uptime.c_str()));
    json_object_set_new(root, "shell", json_string(System::s_Shell.c_str()));
    json_object_set_new(root, "desktop",
                        json_string(System::s_Desktop.c_str()));
    json_object_set_new(root, "window_manager",
                        json_string(System::s_WindowManager.c_str()));

    // hardware
    json_object_set_new(root, "architecture",
                        json_string(System::s_Arch.c_str()));
    json_object_set_new(root, "cpu_freq",
                        json_string(System::s_CpuFreq.c_str()));

    // cpu
    json_object_set_new(root, "cpu_model",
                        json_string(System::s_CpuModel.c_str()));
    json_object_set_new(root, "cpu_usage", json_real(System::s_CpuUsage));

    // core
    json_t *cores = json_array();
    for (float usage : System::s_CpuCore) {
      json_array_append_new(cores, json_real(usage));
    }
    json_object_set_new(root, "cpu_core", cores);

    // ram
    json_object_set_new(root, "ram_total", json_real(System::s_RamTotal));
    json_object_set_new(root, "ram_used", json_real(System::s_RamUsed));
    json_object_set_new(root, "ram_free", json_real(System::s_RamFree));

    // disk
    json_object_set_new(root, "root_disk", json_real(System::s_RootDisk));
    json_object_set_new(root, "swap_disk", json_real(System::s_SwapDisk));

    if (json_dump_file(root, "system.json", JSON_INDENT(4)) != 0) {
      fprintf(stderr, "Failed to write system.json\n");
    }

    // clean
    json_decref(root);

    std::this_thread::sleep_for(1s);
  }
}
