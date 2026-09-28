cmake_minimum_required(VERSION 3.20)
project(AurevixEcho VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Put executables next to each other: build/bin/Release/... on multi-config,
# or build/bin/... on single-config generators.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY         ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release)

if(NOT WIN32)
  message(FATAL_ERROR "Aurevix Echo currently targets Windows (Win32 + DX11).")
endif()

if(MSVC)
  add_compile_options(/W4 /permissive- /Zc:__cplusplus /utf-8 /MP)
  add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)

# ---------- SQLite ----------
find_package(SQLite3 QUIET)
if(NOT SQLite3_FOUND)
  include(FetchContent)
  FetchContent_Declare(sqlite3
    URL https://www.sqlite.org/2024/sqlite-amalgamation-3450300.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  FetchContent_MakeAvailable(sqlite3)
  add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
  target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})
  target_compile_definitions(sqlite3 PRIVATE SQLITE_THREADSAFE=1)
  add_library(SQLite::SQLite3 ALIAS sqlite3)
endif()

# ---------- Dear ImGui ----------
include(FetchContent)
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.90.9)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
  ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp)

target_include_directories(imgui PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
  ${imgui_SOURCE_DIR}/misc/cpp)

target_link_libraries(imgui PUBLIC d3d11 dxgi dxguid)

# ---------- Aurevix Echo ----------
add_executable(AurevixEcho
  src/main.cpp
  src/core/Logger.cpp
  src/core/Configuration.cpp
  src/core/Application.cpp
  src/storage/Database.cpp
  src/storage/Repositories.cpp
  src/monitoring/WindowMonitor.cpp
  src/monitoring/SystemMonitor.cpp
  src/monitoring/ActivityMonitor.cpp
  src/analytics/SessionAnalyzer.cpp
  src/analytics/PatternDetector.cpp
  src/graph/WorkflowGraph.cpp
  src/utils/Export.cpp
  src/ui/Theme.cpp
  src/ui/UI.cpp)

# =====================================================================
#  FIX (LNK2019): This target uses wWinMain as its entry point, so it
#  must be linked as a Windows GUI application, not a console one.
#  Without this, CMake links against mainCRTStartup which looks for
#  `main` and fails with "unresolved external symbol main".
# =====================================================================
if(MSVC)
  set_target_properties(AurevixEcho PROPERTIES
    WIN32_EXECUTABLE TRUE
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:AurevixEcho>"
  )
else()
  set_target_properties(AurevixEcho PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_include_directories(AurevixEcho PRIVATE src)

target_link_libraries(AurevixEcho PRIVATE
  imgui
  SQLite::SQLite3
  Threads::Threads
  d3d11 dxgi dxguid
  pdh iphlpapi psapi
  version shlwapi)

target_compile_definitions(AurevixEcho PRIVATE
  NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE
  _WIN32_WINNT=0x0601
  WINVER=0x0601
  AUR_VERSION="${PROJECT_VERSION}")