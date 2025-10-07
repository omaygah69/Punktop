#include "../include/implot/implot.h"
#include "../punktop.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <sys/statvfs.h>
#include <thread>
#include <vector>

static std::atomic<bool> disk_is_finished{false};
static const size_t DISK_HISTORY_SIZE = 120; // 2 minutes at 1s interval
static std::vector<float> disk_history;
static std::mutex disk_mutex;

struct DiskStat {
    float used_gb = 0.0f;
    float total_gb = 0.0f;
};

DiskStat ReadDiskStat(const char *path = "/") {
    DiskStat stat;
    struct statvfs vfs;
    if (statvfs(path, &vfs) != 0) {
        return stat; // error → return zeros
    }

    unsigned long long total = vfs.f_blocks * vfs.f_frsize;
    unsigned long long free = vfs.f_bfree * vfs.f_frsize;
    unsigned long long used = total - free;

    stat.total_gb = total / (1024.0f * 1024.0f * 1024.0f);
    stat.used_gb = used / (1024.0f * 1024.0f * 1024.0f);

    return stat;
}

void FetchDiskUsage() {
    while (!disk_is_finished) {
        DiskStat stat = ReadDiskStat("/"); // root filesystem

        float usage_percent =
            (stat.total_gb > 0.0f) ? (stat.used_gb / stat.total_gb) * 100.0f : 0.0f;

        {
            std::lock_guard<std::mutex> lock(disk_mutex);
            if (disk_history.size() >= DISK_HISTORY_SIZE) {
                disk_history.erase(disk_history.begin());
            }
            disk_history.push_back(usage_percent);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(
            static_cast<int>(read_speed * 1000)));
    }
}

void ShowDiskUsage() {
    std::vector<float> disk;
    {
        std::lock_guard<std::mutex> lock(disk_mutex);
        disk = disk_history;
    }

    if (disk.empty())
        return;

    ImGui::BeginChild("DiskUsagePanel", ImVec2(0, 0), true);
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Disk Usage (Root FS)");
    ImGui::Text("Current: %.1f%%", disk.back());
    ImGui::Spacing();

    if (ImPlot::BeginPlot("##DiskPlot", ImVec2(-1, 200),
                          ImPlotFlags_NoTitle | ImPlotFlags_NoLegend |
                          ImPlotFlags_NoMouseText | ImPlotFlags_CanvasOnly)) {
        ImPlot::SetupAxes("Time", "% Usage", ImPlotAxisFlags_NoTickLabels,
                          ImPlotAxisFlags_NoTickLabels);
        ImPlot::SetupAxesLimits(0, DISK_HISTORY_SIZE, 0, 100, ImGuiCond_Always);

        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.9f, 0.7f, 0.3f, 1.0f));
        ImPlot::PlotLine("Disk", disk.data(), (int)disk.size());
        ImPlot::PushStyleColor(ImPlotCol_Fill, ImVec4(0.9f, 0.7f, 0.3f, 0.2f));
        ImPlot::PlotShaded("Disk", disk.data(), (int)disk.size(), 0.0f);
        ImPlot::PopStyleColor(2);

        ImPlot::EndPlot();
    }

    ImGui::EndChild();
}
