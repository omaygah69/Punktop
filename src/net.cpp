#include "../punktop.h"
#include "../include/implot/implot.h"
#include <fstream>
#include <sstream>
#include <chrono>

static std::atomic<bool> net_is_finished{false};
static const size_t HISTORY_SIZE = 120; // ~2 minutes at 1s interval
static NetHistory net_history;
static std::mutex net_mutex;

NetStat ReadNetStat(const std::string& iface) {
    NetStat stat;
    std::ifstream file("/proc/net/dev");
    if (!file.is_open()) return stat;

    std::string line;
    while (std::getline(file, line)) {
        if (line.find(iface + ":") != std::string::npos) {
            std::istringstream iss(line.substr(line.find(":") + 1));
            iss >> stat.rx_bytes;
            for (int i = 0; i < 7; i++) iss >> std::ws; // skip other rx fields
            iss >> stat.tx_bytes;
            break;
        }
    }
    return stat;
}

void GetNetworkUsage(const std::string& iface) {
    NetStat prev = ReadNetStat(iface);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    while (!net_is_finished) {
        NetStat curr = ReadNetStat(iface);

        float rx_kb = (curr.rx_bytes - prev.rx_bytes) / 1024.0f;
        float tx_kb = (curr.tx_bytes - prev.tx_bytes) / 1024.0f;

        {
            std::lock_guard<std::mutex> lock(net_mutex);
            if (net_history.rx_history.size() >= HISTORY_SIZE) {
                net_history.rx_history.erase(net_history.rx_history.begin());
                net_history.tx_history.erase(net_history.tx_history.begin());
            }
            net_history.rx_history.push_back(rx_kb);
            net_history.tx_history.push_back(tx_kb);
        }

        prev = curr;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void ShowNetworkUsage(float height) {
    std::vector<float> rx, tx;
    {
        std::lock_guard<std::mutex> lock(net_mutex);
        rx = net_history.rx_history;
        tx = net_history.tx_history;
    }

    if (rx.empty() || tx.empty())
        return;

    ImGui::BeginChild("NetUsagePanel", ImVec2(0, height), true);

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Network Usage");

    // Current values
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
    ImGui::Text("RX: %.1f KB/s", rx.back());
    ImGui::PopStyleColor();

    ImGui::SameLine(200);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.4f, 0.4f, 1.0f));
    ImGui::Text("TX: %.1f KB/s", tx.back());
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // Shaded plot fills
    if (ImPlot::BeginPlot("##NetPlot", ImVec2(-1,200),
                          ImPlotFlags_NoTitle | ImPlotFlags_NoLegend |
                          ImPlotFlags_NoMouseText | ImPlotFlags_CanvasOnly)) {
        ImPlot::SetupAxes("Wifi Bruh", "KB/s",
                          ImPlotAxisFlags_NoTickLabels,
                          ImPlotAxisFlags_NoTickLabels);
        ImPlot::SetupAxesLimits(0, HISTORY_SIZE, 0, 2000, ImGuiCond_Always);

        // RX green line + shaded area
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
        ImPlot::PlotLine("RX", rx.data(), (int)rx.size());
        ImPlot::PushStyleColor(ImPlotCol_Fill, ImVec4(0.3f, 0.9f, 0.3f, 0.2f));
        ImPlot::PlotShaded("RX", rx.data(), (int)rx.size(), 0.0f);
        ImPlot::PopStyleColor(2);

        // TX red line + shaded area
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.9f, 0.4f, 0.4f, 1.0f));
        ImPlot::PlotLine("TX", tx.data(), (int)tx.size());
        ImPlot::PushStyleColor(ImPlotCol_Fill, ImVec4(0.9f, 0.4f, 0.4f, 0.2f));
        ImPlot::PlotShaded("TX", tx.data(), (int)tx.size(), 0.0f);
        ImPlot::PopStyleColor(2);

        ImPlot::EndPlot();
    }

    ImGui::EndChild();
}
