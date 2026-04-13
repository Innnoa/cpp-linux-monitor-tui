cmake_minimum_required(VERSION 3.20)
project(cpp_linux_monitor_tui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
    include(FetchContent)
    FetchContent_Declare(
      catch2
      GIT_REPOSITORY https://github.com/catchorg/Catch2.git
      GIT_TAG v3.7.1
    )
    FetchContent_MakeAvailable(catch2)
    set(CATCH_EXTRA_MODULE_DIR ${catch2_SOURCE_DIR}/extras)
  endif()

  if(CATCH_EXTRA_MODULE_DIR)
    list(APPEND CMAKE_MODULE_PATH ${CATCH_EXTRA_MODULE_DIR})
  endif()

  include(Catch)
  add_executable(monitor_tests tests/app/app_config_test.cpp)
  target_link_libraries(monitor_tests PRIVATE monitor_core Catch2::Catch2WithMain)
  catch_discover_tests(monitor_tests)
endif()
