#include "app/application.h"

#include <iostream>

namespace monitor::app {

Application::Application(AppConfig config)
    : config_(config), store_(config.history_size), controller_(config) {}

int Application::run() {
    std::cout << "replace bootstrap with FTXUI event loop\n";
    return 0;
}

}  // namespace monitor::app
