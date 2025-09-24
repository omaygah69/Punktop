#include "../punktop.h"
#include "../include/implot/implot.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <vector>
#include <mutex>
#include <atomic>

static std::atomic<bool> cpu_is_finished{false};
static const size_t HISTORY_SIZE = 120; // 2 minutes at 1s interval
static std::vector<float> cpu_history;
static std::mutex cpu_mutex;

struct CpuStat
{
    long user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0;
    long total() const { return user + nice + system + idle + iowait + irq + softirq; }
};

CpuStat ReadCpuStat()
{
    std::ifstream file("/proc/stat");
    CpuStat stat;
    if (!file.is_open()) return stat;

    std::string line;
    std::getline(file, line);
    if (line.substr(0, 3) != "cpu") return stat;

    std::istringstream iss(line);
    std::string cpu_label;
    iss >> cpu_label >> stat.user >> stat.nice >> stat.system >> stat.idle 
        >> stat.iowait >> stat.irq >> stat.softirq;

    return stat;
}

void FetchCpuUsageForPlot()
{
    CpuStat prev_stat = ReadCpuStat();
    while (!cpu_is_finished) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        CpuStat curr_stat = ReadCpuStat();

        long prev_total = prev_stat.total();
        long curr_total = curr_stat.total();
        long total_diff = curr_total - prev_total;
        long idle_diff = curr_stat.idle - prev_stat.idle;

        float usage_percent = (total_diff > 0)
            ? 100.0f * (total_diff - idle_diff) / total_diff
            : 0.0f;

        {
            std::lock_guard<std::mutex> lock(cpu_mutex);
            if (cpu_history.size() >= HISTORY_SIZE) {
                cpu_history.erase(cpu_history.begin());
            }
            cpu_history.push_back(usage_percent);
        }

        prev_stat = curr_stat;
    }
}

void ShowCpuPlot(float height)
{
    std::vector<float> cpu;
    {
        std::lock_guard<std::mutex> lock(cpu_mutex);
        cpu = cpu_history;
    }

    if (cpu.empty())
        return;

    ImGui::BeginChild("CpuUsagePanel", ImVec2(0, height), true);
    ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "CPU Usage");
    ImGui::Text("Current: %.1f%%", cpu.back());
    ImGui::Spacing();

    if (ImPlot::BeginPlot("##CpuPlot", ImVec2(-1, 200),
                          ImPlotFlags_NoTitle | ImPlotFlags_NoLegend |
                          ImPlotFlags_NoMouseText | ImPlotFlags_CanvasOnly)) {
        ImPlot::SetupAxes("Time", "% Usage",
                          ImPlotAxisFlags_NoTickLabels,
                          ImPlotAxisFlags_NoTickLabels);
        ImPlot::SetupAxesLimits(0, HISTORY_SIZE, 0, 100, ImGuiCond_Always);

        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        ImPlot::PlotLine("CPU", cpu.data(), (int)cpu.size());
        ImPlot::PushStyleColor(ImPlotCol_Fill, ImVec4(0.9f, 0.3f, 0.3f, 0.2f));
        ImPlot::PlotShaded("CPU", cpu.data(), (int)cpu.size(), 0.0f);
        ImPlot::PopStyleColor(2);

        ImPlot::EndPlot();
    }
    ImGui::EndChild();
}
