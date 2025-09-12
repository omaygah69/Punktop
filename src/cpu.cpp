#include "../punktop.h"
#include <chrono>
#define PATH "/proc/stat"
#define BUFFER_SIZE 256

static std::vector<float> cpu_usage_list;
std::atomic<bool> is_finished{false};
std::atomic<bool> fetch_finished = false;
static void ReadCpuModel();

void ParseCpuLine(const char* line, Core_t* ct)
{
    sscanf(line,
           "%*s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
           &ct->user, &ct->nice, &ct->system, &ct->idle, &ct->iowait,
           &ct->irq, &ct->softirq, &ct->steal, &ct->guest, &ct->guest_nice);
}

static unsigned long long IdleTime(const Core_t* ct) {
    return ct->idle + ct->iowait;
}

static unsigned long long TotalTime(const Core_t* ct) {
    return ct->user + ct->nice + ct->system + ct->idle + ct->iowait + ct->irq + ct->softirq + ct->steal;
}

float Normalize(float value)
{
    float t = (value - 1.0f) / (100.0f - 1.0f);
    // optional clamp
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t;
}

std::vector<Core_t> ReadCpuStatusFile()
{
    FILE* fp = fopen(PATH, "r");
    if(fp == nullptr){
        fprintf(stderr, "[ERROR] Failed opening path %s", PATH);
        exit(EXIT_FAILURE);
    }
    std::vector<Core_t> entries;
    char buffer[BUFFER_SIZE];
    int index = 0;
    while(fgets(buffer, sizeof(buffer), fp)){
        if(strncmp(buffer, "cpu", 3) == 0){
            if(index > MAX_CORES) break;
            Core_t ct = {0};
            ParseCpuLine(buffer, &ct);
            entries.push_back(ct);
            index++;
        } else { break; }
    }
    fclose(fp);
    return entries;
}

std::vector<float> CalculateCpuUsage(const std::vector<Core_t>& prev, const std::vector<Core_t>& curr)
{
    // Check this out blyat
    std::vector<float> entries;
    size_t count = std::min(curr.size(), prev.size());
    for(int i = 0 ; i < count; i++){
        size_t prev_total = TotalTime(&prev[i]);
        size_t curr_total = TotalTime(&curr[i]);
        size_t prev_idle = IdleTime(&prev[i]);
        size_t curr_idle = IdleTime(&curr[i]);
        float total_diff= static_cast<float>(curr_total - prev_total);
        float idle_diff= static_cast<float>(curr_idle - prev_idle);
        float cpu_usage = 100.0f * (total_diff - idle_diff) / total_diff;
        entries.push_back(cpu_usage);
    }

    return entries;
}

void ShowCpuUsage()
{
    if (cpu_usage_list.empty())
        return;

    ImGui::BeginChild("CPUUsagePanel", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);
    // ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "CPU Usage Overview");
    ReadCpuModel();
    // Total CPU
    {
        float total = cpu_usage_list[0];
        float norm = Normalize(total);
        ImVec4 color;
        if (total > 80.0f) color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);     // Red
        else if (total > 50.0f) color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // Yellow
        else color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);                   // Green

        ImGui::Text("Total CPU: %.1f%%", total);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
        ImGui::ProgressBar(norm, ImVec2(-FLT_MIN, 18.0f)); // full width
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    // Per-core
    for (int i = 1; i < cpu_usage_list.size(); ++i)
    {
        float usage = cpu_usage_list[i];
        float norm = Normalize(usage);

        ImVec4 color;
        if (usage > 80.0f) color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        else if (usage > 50.0f) color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
        else color = ImVec4(0.3f, 1.0f, 0.9f, 1.0f);

        ImGui::Text("Core %d:", i - 1);
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
        ImGui::ProgressBar(norm, ImVec2(150.0f, 14.0f));
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text("%.1f%%", usage);
    }

    ImGui::EndChild();
}

void GetCpuUsage()
{
    using clock = std::chrono::steady_clock;
    auto last_time = clock::now();
    while (!is_finished) {
        auto now = clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time);
        if (elapsed < std::chrono::milliseconds(2000)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        last_time = now;
        auto prev = ReadCpuStatusFile();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto curr = ReadCpuStatusFile();
        cpu_usage_list = CalculateCpuUsage(prev, curr);
        fetch_finished.store(true, std::memory_order_release);
    }
}

static void ReadCpuModel(){
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) {
        perror("fopen");
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "model name", 10) == 0) {
            char *colon = strchr(line, ':');
            if (colon) {
                // printf("CPU: %s", colon + 2); // skip ": "
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "CPU: %s", colon + 2);
            }
            break; 
        }
    }
    fclose(fp);
}
