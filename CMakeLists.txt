cmake_minimum_required(VERSION 3.20)
project(cpp_linux_monitor_tui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTING "Build tests" OFF)
option(MONITOR_FETCH_DEPS "Allow CMake to fetch missing third-party dependencies" OFF)

include(CTest)

set(MONITOR_HAS_FTXUI OFF)
set(MONITOR_HAS_FTXUI_COMPONENT OFF)
find_package(ftxui CONFIG QUIET)
if(TARGET ftxui::dom AND TARGET ftxui::screen)
  set(MONITOR_HAS_FTXUI ON)
endif()
if(TARGET ftxui::component)
  set(MONITOR_HAS_FTXUI_COMPONENT ON)
endif()

if(NOT MONITOR_HAS_FTXUI AND MONITOR_FETCH_DEPS)
  include(FetchContent)
  FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG v5.0.0
  )
  FetchContent_MakeAvailable(ftxui)
  if(TARGET ftxui::dom AND TARGET ftxui::screen)
    set(MONITOR_HAS_FTXUI ON)
  endif()
  if(TARGET ftxui::component)
    set(MONITOR_HAS_FTXUI_COMPONENT ON)
  endif()
endif()

if(NOT MONITOR_HAS_FTXUI)
  if(ftxui_FOUND)
    message(STATUS "FTXUI package found but required dom/screen targets missing; dashboard renderer will be disabled.")
  else()
    message(STATUS "FTXUI not found; dashboard renderer will be disabled.")
  endif()
endif()

find_package(Catch2 3 CONFIG QUIET)
set(CATCH_EXTRA_MODULE_DIR "")
if(Catch2_FOUND AND Catch2_DIR)
  set(CATCH_EXTRA_MODULE_DIR ${Catch2_DIR}/extras)
endif()

set(MONITOR_CORE_SOURCES
  src/store/snapshot_store.cpp
  src/app/application.cpp
  src/app/sampling_worker.cpp
  src/collector/cpu_collector.cpp
  src/collector/disk_collector.cpp
  src/collector/memory_collector.cpp
  src/collector/network_collector.cpp
  src/collector/process_collector.cpp
  src/actions/process_actions.cpp
  src/ui/app_controller.cpp)
if(MONITOR_HAS_FTXUI)
  list(APPEND MONITOR_CORE_SOURCES src/ui/dashboard_view.cpp)
endif()

add_library(monitor_core STATIC ${MONITOR_CORE_SOURCES})
target_include_directories(monitor_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(MONITOR_HAS_FTXUI)
  target_compile_definitions(monitor_core PUBLIC MONITOR_HAS_FTXUI=1)
else()
  target_compile_definitions(monitor_core PUBLIC MONITOR_HAS_FTXUI=0)
endif()
if(MONITOR_HAS_FTXUI)
  target_link_libraries(monitor_core PUBLIC ftxui::dom ftxui::screen)
endif()

add_executable(monitor_tui src/main.cpp)
target_link_libraries(monitor_tui PRIVATE monitor_core)

if(BUILD_TESTING)
  if(NOT Catch2_FOUND)
    if(NOT MONITOR_FETCH_DEPS)
      message(FATAL_ERROR "Catch2 not found. Install Catch2 or rerun CMake with -DBUILD_TESTING=ON -DMONITOR_FETCH_DEPS=ON.")
    endif()

    include(FetchContent)
    FetchContent_Declare(
      catch2
      GIT_REPOSITORY https://github.com/catchorg/Catch2.git
      GIT_TAG v3.7.1
    )
    FetchContent_MakeAvailable(catch2)
    set(CATCH_EXTRA_MODULE_DIR ${catch2_SOURCE_DIR}/extras)
  endif()

  set(MONITOR_USE_CATCH_MODULE OFF)
  if(CATCH_EXTRA_MODULE_DIR)
    list(APPEND CMAKE_MODULE_PATH ${CATCH_EXTRA_MODULE_DIR})
    include(Catch)
    set(MONITOR_USE_CATCH_MODULE ON)
  endif()

  set(MONITOR_TEST_SOURCES
    tests/app/app_config_test.cpp
    tests/app/application_test.cpp
    tests/store/snapshot_store_test.cpp
    tests/collector/system_collectors_test.cpp
    tests/collector/process_collector_test.cpp
    tests/actions/process_actions_test.cpp
    tests/ui/app_controller_test.cpp)
  if(MONITOR_HAS_FTXUI)
    list(APPEND MONITOR_TEST_SOURCES tests/ui/dashboard_view_test.cpp)
  else()
    message(STATUS "FTXUI not found; dashboard_view_test disabled.")
  endif()

  add_executable(monitor_tests ${MONITOR_TEST_SOURCES})
  target_link_libraries(monitor_tests PRIVATE monitor_core Catch2::Catch2WithMain)
  if(MONITOR_HAS_FTXUI_COMPONENT)
    target_link_libraries(monitor_tests PRIVATE ftxui::component)
  endif()

  if(MONITOR_USE_CATCH_MODULE)
    catch_discover_tests(monitor_tests)
  else()
    add_test(NAME monitor_tests COMMAND monitor_tests)
  endif()
endif()
