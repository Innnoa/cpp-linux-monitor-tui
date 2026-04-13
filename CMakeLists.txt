cmake_minimum_required(VERSION 3.28)
project(cpp_linux_monitor_tui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
include(CTest)

FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
  GIT_TAG v5.0.0
)

FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.7.1
)

FetchContent_MakeAvailable(ftxui catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

add_library(monitor_core INTERFACE)
target_include_directories(monitor_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(monitor_tui src/main.cpp)
target_link_libraries(monitor_tui PRIVATE monitor_core ftxui::screen ftxui::dom ftxui::component)

include(Catch)
add_executable(monitor_tests tests/app/app_config_test.cpp)
target_link_libraries(monitor_tests PRIVATE monitor_core Catch2::Catch2WithMain)
catch_discover_tests(monitor_tests)
