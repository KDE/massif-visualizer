cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
project(massif-visualizer)

set(QT_MIN_VERSION "5.2.0")

find_package(ECM 0.0.9 REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/
    ${CMAKE_MODULE_PATH}
    ${ECM_MODULE_PATH}
    ${ECM_KDE_MODULE_DIR}
)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMOptionalAddSubdirectory)
include(ECMInstallIcons)
include(FeatureSummary)

enable_testing()

find_package(KGraphViewer 2.1)
#macro_log_feature(KGRAPHVIEWER_FOUND "KGraphViewer" "KPart to view callgraph dot files."
#                  "http://extragear.kde.org/apps/kgraphviewer/" FALSE ""
#                  "Required for detailed snapshot analysis. From KDE SDK Extragear package, KGraphViewer 2.1 or higher is required.")

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED
    Core
    Widgets
    Svg
    XmlPatterns
)

find_package(KF5 REQUIRED COMPONENTS
    Archive
    Config
    CoreAddons
    Parts
    KIO
    I18n
    KDELibs4Support
)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/kdchart/include/KDChart
)

add_definitions(-Wall)

add_subdirectory(kdchart)
add_subdirectory(massifdata)
add_subdirectory(visualizer)
add_subdirectory(app)
add_subdirectory(pics)

# TODO: reenable tests
#add_subdirectory(test)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
