#pragma once

#include <cstdint>
#include <string_view>

#include "model/system_snapshot.h"

namespace monitor::collector {

model::MemoryMetrics parse_memory_info(std::string_view text);

}  // namespace monitor::collector
