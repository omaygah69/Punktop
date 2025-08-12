#include "../punktop.h"
#include <fstream>
#include <sstream>
#include <chrono>

static std::atomic<bool> net_is_finished{false};
static const size_t HISTORY_SIZE = 120; // ~2 mins at 1s interval

// Double buffer: index 0 = front (read), 1 = back (write)
static NetData net_buffers[2];
static std::atomic<int> net_front_index{0};

// Reads /proc/net/dev and returns NetStat for an Interface
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
            iss >> stat.tx_bytes; // after skipping rx fields, land on tx
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

        // Choose back buffer (not currently displayed)
        int back_index = 1 - net_front_index.load(std::memory_order_relaxed);
        NetData& back = net_buffers[back_index];

        back.stat = curr;
        if (back.rx_history.size() >= HISTORY_SIZE) back.rx_history.erase(back.rx_history.begin());
        if (back.tx_history.size() >= HISTORY_SIZE) back.tx_history.erase(back.tx_history.begin());
        back.rx_history.push_back(rx_kb);
        back.tx_history.push_back(tx_kb);

        prev = curr;

        // Flip buffers
        net_front_index.store(back_index, std::memory_order_release);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void ShowNetworkUsage() {
    int front_index = net_front_index.load(std::memory_order_acquire);
    const NetData& front = net_buffers[front_index];

    if (front.rx_history.empty() || front.tx_history.empty())
        return;

    ImGui::BeginChild("NetUsagePanel", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);

    // Title
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Network Usage");

    // Current usage row
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.3f, 1.0f)); // green for RX
    ImGui::Text("RX: %.1f KB/s", front.rx_history.back());
    ImGui::PopStyleColor();

    ImGui::SameLine(200);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.4f, 0.4f, 1.0f)); // red for TX
    ImGui::Text("TX: %.1f KB/s", front.tx_history.back());
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // RX plot
    ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
    ImGui::PlotLines("##rxplot",
                     front.rx_history.data(),
                     static_cast<int>(front.rx_history.size()),
                     0, "RX History",
                     0.0f, 2000.0f, ImVec2(-1, 60));
    ImGui::PopStyleColor();

    // TX plot
    ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.9f, 0.4f, 0.4f, 1.0f));
    ImGui::PlotLines("##txplot",
                     front.tx_history.data(),
                     static_cast<int>(front.tx_history.size()),
                     0, "TX History",
                     0.0f, 2000.0f, ImVec2(-1, 60));
    ImGui::PopStyleColor();

    ImGui::EndChild();
}
