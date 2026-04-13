#pragma once

#include "app/app_config.h"
#include "store/snapshot_store.h"
#include "ui/app_controller.h"

namespace monitor::app {

class Application {
  public:
    explicit Application(AppConfig config);
    int run();

  private:
    AppConfig config_;
    store::SnapshotStore store_;
    ui::AppController controller_;
};

}  // namespace monitor::app
