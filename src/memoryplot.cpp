#include "../include/implot/implot.h"
#include "../punktop.h"
#include "system.h"
#include <chrono>
#include <fstream>
#include <sstream>

static std::atomic<bool> mem_is_finished{false};
static const size_t HISTORY_SIZE = 120; // 2 minutes at 1s interval
static std::vector<float> mem_history;
static std::mutex mem_mutex;

struct MemStat {
    float used_mb = 0.0f;
    float total_mb = 0.0f;
};

MemStat ReadMemStat() {
    std::ifstream file("/proc/meminfo");
    MemStat stat;
    if (!file.is_open())
        return stat;

    std::string key;
    long value;
    std::string unit;
    long mem_total = 0, mem_free = 0, buffers = 0, cached = 0;

    while (file >> key >> value >> unit) {
        if (key == "MemTotal:")
            mem_total = value;
        else if (key == "MemFree:")
            mem_free = value;
        else if (key == "Buffers:")
            buffers = value;
        else if (key == "Cached:")
            cached = value;
    }

    // Values are in KB
    long used = mem_total - (mem_free + buffers + cached);

    stat.total_mb = mem_total / 1024.0f;
    stat.used_mb = used / 1024.0f;
    System::s_RamTotal = stat.total_mb;
    System::s_RamUsed = stat.used_mb;
    System::s_RamFree = stat.total_mb - stat.used_mb;

    return stat;
}

void FetchMemoryUsage() {
    while (!mem_is_finished) {
        MemStat stat = ReadMemStat();

        float usage_percent =
            (stat.total_mb > 0.0f) ? (stat.used_mb / stat.total_mb) * 100.0f : 0.0f;

        {
            std::lock_guard<std::mutex> lock(mem_mutex);
            if (mem_history.size() >= HISTORY_SIZE) {
                mem_history.erase(mem_history.begin());
            }
            mem_history.push_back(usage_percent);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(
            static_cast<int>(read_speed * 1000)));
    }
}

void ShowMemoryUsage(float height) {
    std::vector<float> mem;
    {
        std::lock_guard<std::mutex> lock(mem_mutex);
        mem = mem_history;
    }

    if (mem.empty())
        return;

    ImGui::BeginChild("MemUsagePanel", ImVec2(0, height), true);
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Memory Usage");
    ImGui::Text("Current: %.1f%%", mem.back());
    ReadMemInfo();
    ImGui::Spacing();

    if (ImPlot::BeginPlot("##MemPlot", ImVec2(-1, 200),
                          ImPlotFlags_NoTitle | ImPlotFlags_NoLegend |
                          ImPlotFlags_NoMouseText | ImPlotFlags_CanvasOnly)) {
        ImPlot::SetupAxes("Time", "% Usage", ImPlotAxisFlags_NoTickLabels,
                          ImPlotAxisFlags_NoTickLabels);
        ImPlot::SetupAxesLimits(0, HISTORY_SIZE, 0, 100, ImGuiCond_Always);

        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
        ImPlot::PlotLine("Mem", mem.data(), (int)mem.size());
        ImPlot::PushStyleColor(ImPlotCol_Fill, ImVec4(0.3f, 0.6f, 0.9f, 0.2f));
        ImPlot::PlotShaded("Mem", mem.data(), (int)mem.size(), 0.0f);
        ImPlot::PopStyleColor(2);

        ImPlot::EndPlot();
    }
    ImGui::EndChild();
}
