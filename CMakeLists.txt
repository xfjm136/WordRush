cmake_minimum_required(VERSION 3.24)

project(WordRush VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
  GIT_TAG v6.1.9
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(ftxui)

find_package(SQLite3 QUIET)

add_library(wordrush_core
  src/core/companion.cpp
  src/core/training.cpp
  src/core/types.cpp
  src/storage/wordbook_repository.cpp
  src/tui/dashboard.cpp
  src/tui/drill_session.cpp
  src/app/application.cpp
)

target_include_directories(wordrush_core PUBLIC include)
target_compile_features(wordrush_core PUBLIC cxx_std_20)
target_compile_definitions(wordrush_core PUBLIC WORDRUSH_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(wordrush_core
  PUBLIC
    ftxui::component
    ftxui::dom
    ftxui::screen
)

if(SQLite3_FOUND)
  target_link_libraries(wordrush_core PUBLIC SQLite3::SQLite3)
  target_compile_definitions(wordrush_core PUBLIC WORDRUSH_HAS_SQLITE=1)
endif()

add_executable(wordrush src/main.cpp)
target_link_libraries(wordrush PRIVATE wordrush_core)

install(TARGETS wordrush RUNTIME DESTINATION bin)
