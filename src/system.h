#pragma once
#include <string>
#include <vector>

struct System {
  // system
  static std::string s_HostName;
  static std::string s_OS;
  static std::string s_Kernel;
  static std::string s_Uptime;

  //
  static std::string s_Shell;
  static std::string s_Desktop;
  static std::string s_WindowManager;

  // hardware
  static std::string s_Arch;
  static std::string s_CpuFreq;

  static std::string s_CpuModel;
  static float s_CpuUsage;
  static std::vector<float> s_CpuCore;

  // ram
  static float s_RamTotal;
  static float s_RamUsed;
  static float s_RamFree;

  // disk
  static float s_RootDisk;
  static float s_SwapDisk;
};
