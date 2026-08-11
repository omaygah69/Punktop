#include "implot.h"
#include "../punktop.h"
#include <atomic>
#include <fstream>
#include <mutex>
#include <sstream>
#include <vector>

static std::atomic<bool> cpu_is_finished{false};
static const size_t HISTORY_SIZE = 120; // 2 minutes at 1s interval
static std::vector<float> cpu_history;
static std::mutex cpu_mutex;

struct CpuStat {
    long user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0,
        softirq = 0;
    long total() const {
        return user + nice + system + idle + iowait + irq + softirq;
    }
};

CpuStat ReadCpuStat() {
    std::ifstream file("/proc/stat");
    CpuStat stat;
    if (!file.is_open())
        return stat;

    std::string line;
    std::getline(file, line);
    if (line.substr(0, 3) != "cpu")
        return stat;

    std::istringstream iss(line);
    std::string cpu_label;
    iss >> cpu_label >> stat.user >> stat.nice >> stat.system >> stat.idle >>
        stat.iowait >> stat.irq >> stat.softirq;

    return stat;
}

void FetchCpuUsageForPlot() {
    CpuStat prev_stat = ReadCpuStat();
    while(!cpu_is_finished) {
        std::this_thread::sleep_for(std::chrono::milliseconds(
            static_cast<int>(read_speed * 1000)));
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

void ShowCpuPlot(float height) {
    std::vector<float> cpu;
    {
        std::lock_guard<std::mutex> lock(cpu_mutex);
        cpu = cpu_history;
    }

    if (cpu.empty())
        return;

    ImVec4 cpu_color(0.9f, 0.3f, 0.3f, 1.0f);
    ImGui::BeginChild("CpuUsagePanel", ImVec2(0, height), true);
    ImGui::TextColored(cpu_color, "CPU Usage");
    ImGui::Text("Current: %.1f%%", cpu.back());
    ImGui::Spacing();

    if (ImPlot::BeginPlot("##CpuPlot", ImVec2(-1, 200),
                          ImPlotFlags_NoTitle | ImPlotFlags_NoLegend |
                          ImPlotFlags_NoMouseText | ImPlotFlags_CanvasOnly)) {
        ImPlot::SetupAxes("Time", "% Usage", ImPlotAxisFlags_NoTickLabels,
                          ImPlotAxisFlags_NoTickLabels);
        ImPlot::SetupAxesLimits(0, HISTORY_SIZE, 0, 100, ImGuiCond_Always);

	// [INFO] Read and review changes before deletion.
        // ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        // ImPlot::PlotLine("CPU", cpu.data(), (int)cpu.size());
        // ImPlot::PushStyleColor(ImPlotCol_Fill, ImVec4(0.9f, 0.3f, 0.3f, 0.2f));
        // ImPlot::PlotShaded("CPU", cpu.data(), (int)cpu.size(), 0.0f);
        // ImPlot::PopStyleColor(2);

	// [INFO] Read changes for PlotShaded
	// ImPlot::PlotShaded's single-array overload should be (label, values, count, y_ref, xscale, xstart, spec)
	// I wrote it like this to show ImPLotSpec
	ImPlotSpec line_spec;
	line_spec.LineColor = cpu_color;
	ImPlot::PlotLine("CPU", cpu.data(), (int)cpu.size(), 1.0, 0.0, line_spec);
        // ImPlot::PlotLine("CPU", cpu.data(), (int)cpu.size(),
        //                  {ImPlotProp_LineColor, cpu_color});
        ImPlot::PlotShaded("CPU", cpu.data(), (int)cpu.size(), 0.0f, 1.0f, 0.0f,
                           {ImPlotProp_FillColor, ImVec4(cpu_color.x, cpu_color.y, cpu_color.z, 0.2f)});


        ImPlot::EndPlot();
    }
    ImGui::EndChild();
}
