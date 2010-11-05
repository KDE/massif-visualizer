PROJECT(massif-visualizer)

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ ${CMAKE_MODULE_PATH})

find_package(KDE4 REQUIRED)

include (KDE4Defaults)
include (MacroLibrary)

macro_optional_find_package(KGraphViewer 2.1)
macro_log_feature(KGRAPHVIEWER_FOUND "KGraphViewer" "KPart to view callgraph dot files."
                  "http://extragear.kde.org/apps/kgraphviewer/" FALSE ""
                  "Required for detailed snapshot analysis. From KDE SDK Extragear package, KGraphViewer 2.1 or higher is required.")

macro_display_feature_log()

include_directories(
    ${KDE4_INCLUDES}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/kdchart/include
    ${CMAKE_CURRENT_SOURCE_DIR}/kdchart/src
    ${CMAKE_CURRENT_BINARY_DIR}/kdchart/src/
)

add_definitions(-Wall)

add_subdirectory(kdchart)
add_subdirectory(massifdata)
add_subdirectory(visualizer)
add_subdirectory(app)

add_subdirectory(test)
