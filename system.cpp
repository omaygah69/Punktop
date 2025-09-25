#include "system.h"
#include <string>
#include <vector>

std::string System::s_HostName;
std::string System::s_OS;
std::string System::s_Kernel;
std::string System::s_Uptime;

//
std::string System::s_Shell;
std::string System::s_Desktop;
std::string System::s_WindowManager;

// hardware
std::string System::s_Arch;
std::string System::s_CpuFreq;

// cpu
std::string System::s_CpuModel;
float System::s_CpuUsage;
std::vector<float> System::s_CpuCore;

// ram
float System::s_RamTotal;
float System::s_RamUsed;
float System::s_RamFree;

// disk
float System::s_RootDisk;
float System::s_SwapDisk;
