cmake_minimum_required(VERSION 3.20)
project(cpp_linux_monitor_tui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTING "Build tests" OFF)
option(MONITOR_FETCH_DEPS "Allow CMake to fetch missing third-party dependencies" OFF)

include(CTest)

find_package(Catch2 3 CONFIG QUIET)
set(CATCH_EXTRA_MODULE_DIR "")
if(Catch2_FOUND AND Catch2_DIR)
  set(CATCH_EXTRA_MODULE_DIR ${Catch2_DIR}/extras)
endif()

add_library(monitor_core INTERFACE)
target_include_directories(monitor_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

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

  add_executable(monitor_tests tests/app/app_config_test.cpp)
  target_link_libraries(monitor_tests PRIVATE monitor_core Catch2::Catch2WithMain)

  if(MONITOR_USE_CATCH_MODULE)
    catch_discover_tests(monitor_tests)
  else()
    add_test(NAME monitor_tests COMMAND monitor_tests)
  endif()
endif()
