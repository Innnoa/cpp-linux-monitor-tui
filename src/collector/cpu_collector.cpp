#include "collector/cpu_collector.h"

#include <limits>
#include <sstream>
#include <string>

namespace monitor::collector {

namespace {
double percent(const CpuTimes& previous, const CpuTimes& current) {
    const auto previous_total = previous.user + previous.nice + previous.system + previous.idle + previous.iowait;
    const auto current_total = current.user + current.nice + current.system + current.idle + current.iowait;
    const auto total_delta = static_cast<double>(current_total - previous_total);
    const auto previous_idle = previous.idle + previous.iowait;
    const auto current_idle = current.idle + current.iowait;
    const auto idle_delta = static_cast<double>(current_idle - previous_idle);
    return total_delta == 0.0 ? 0.0 : ((total_delta - idle_delta) / total_delta) * 100.0;
}
}  // namespace

CpuSample parse_cpu_sample(std::string_view text) {
    CpuSample sample;
    std::istringstream input(std::string{text});
    std::string label;
    CpuTimes times;

    while (input >> label >> times.user >> times.nice >> times.system >> times.idle >> times.iowait) {
        if (label == "cpu") {
            sample.total = times;
        } else if (label.rfind("cpu", 0) == 0) {
            sample.cores.push_back(times);
        }
        input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    return sample;
}

model::CpuMetrics compute_cpu_metrics(const CpuSample& previous, const CpuSample& current) {
    model::CpuMetrics metrics;
    metrics.total_percent = percent(previous.total, current.total);
    for (std::size_t index = 0; index < current.cores.size(); ++index) {
        CpuTimes fallback_previous;
        const CpuTimes* previous_core = &fallback_previous;
        if (index < previous.cores.size()) {
            previous_core = &previous.cores[index];
        }
        metrics.core_percents.push_back(percent(*previous_core, current.cores[index]));
    }
    return metrics;
}

}  // namespace monitor::collector
