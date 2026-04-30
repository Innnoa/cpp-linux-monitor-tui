#include "ui/render_cache.h"

namespace monitor::ui {

void RenderCache::mark_dirty() {
    dirty_.store(true, std::memory_order_release);
}

bool RenderCache::is_dirty() const {
    return dirty_.load(std::memory_order_acquire);
}

void RenderCache::update_snapshot(const model::SystemSnapshot& snapshot) {
    snapshot_ = snapshot;
    mark_dirty();
}

void RenderCache::update_history(const model::HistoryData& history) {
    history_ = history;
    mark_dirty();
}

const model::SystemSnapshot& RenderCache::cached_snapshot() const {
    return snapshot_;
}

const model::HistoryData& RenderCache::cached_history() const {
    return history_;
}

std::string RenderCache::cached_render_string() const {
    return render_string_;
}

void RenderCache::set_cached_render_string(std::string str) {
    render_string_ = std::move(str);
    dirty_.store(false, std::memory_order_release);
}

void RenderCache::clear() {
    snapshot_ = {};
    history_ = {};
    render_string_.clear();
    dirty_.store(true, std::memory_order_release);
}

}  // namespace monitor::ui
