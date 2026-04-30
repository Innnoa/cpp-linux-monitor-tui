#pragma once

#include <atomic>
#include <chrono>
#include <string>

#include "model/system_snapshot.h"
#include "model/history_data.h"
#include "ui/app_controller.h"

namespace monitor::ui {

class RenderCache {
public:
    using Clock = std::chrono::steady_clock;

    void mark_dirty();
    bool is_dirty() const;

    void update_snapshot(const model::SystemSnapshot& snapshot);
    void update_history(const model::HistoryData& history);

    const model::SystemSnapshot& cached_snapshot() const;
    const model::HistoryData& cached_history() const;

    std::string cached_render_string() const;
    void set_cached_render_string(std::string str);

    void clear();

private:
    mutable std::atomic<bool> dirty_{true};
    model::SystemSnapshot snapshot_;
    model::HistoryData history_;
    std::string render_string_;
    Clock::time_point last_update_;
};

}  // namespace monitor::ui
