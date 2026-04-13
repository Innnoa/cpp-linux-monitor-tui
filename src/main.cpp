#include "app/app_config.h"
#include "app/application.h"

int main() {
    monitor::app::Application app(monitor::app::AppConfig::defaults());
    return app.run();
}
