#pragma once

#include "app/app_config.h"
#include "model/history_data.h"
#include "store/snapshot_store.h"
#include "ui/app_controller.h"
#include "ui/render_cache.h"

namespace monitor::app {

class Application {
  public:
    explicit Application(AppConfig config);
    int run();

  private:
    AppConfig config_;
    store::SnapshotStore store_;
    ui::AppController controller_;
    model::HistoryData history_;
    ui::RenderCache render_cache_;
};

}  // namespace monitor::app
