#include "collector/cpu_collector.h"

#include <limits>
#include <sstream>
#include <string>

namespace monitor::collector {

namespace {
double percent(const CpuTimes& previous, const CpuTimes& current) {
    const auto previous_total = previous.user + previous.nice + previous.system + previous.idle;
    const auto current_total = current.user + current.nice + current.system + current.idle;
    const auto total_delta = static_cast<double>(current_total - previous_total);
    const auto idle_delta = static_cast<double>(current.idle - previous.idle);
    return total_delta == 0.0 ? 0.0 : ((total_delta - idle_delta) / total_delta) * 100.0;
}
}  // namespace

CpuSample parse_cpu_sample(std::string_view text) {
    CpuSample sample;
    std::istringstream input(std::string{text});
    std::string label;
    CpuTimes times;

    while (input >> label >> times.user >> times.nice >> times.system >> times.idle) {
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
        metrics.core_percents.push_back(percent(previous.cores.at(index), current.cores.at(index)));
    }
    return metrics;
}

}  // namespace monitor::collector
