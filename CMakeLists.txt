cmake_minimum_required(VERSION 3.0)
project(massif-visualizer)

set(QT_MIN_VERSION "5.2.0")

find_package(ECM 1.6.0 REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/
    ${CMAKE_MODULE_PATH}
    ${ECM_MODULE_PATH}
)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMOptionalAddSubdirectory)
include(ECMInstallIcons)
include(ECMAddTests)
include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED
    Core
    Widgets
    Svg
    Test
    PrintSupport
)

find_package(KF5 REQUIRED COMPONENTS
    Archive
    Config
    CoreAddons
    Parts
    KIO
    I18n
)

find_package(KChart "2.6.0" REQUIRED)

find_package(KGraphViewerPart 2.3.90)
set_package_properties(KGraphViewerPart PROPERTIES
    DESCRIPTION "KPart to view callgraph dot files"
    PURPOSE "Required for detailed snapshot analysis"
    URL "https://www.kde.org/applications/graphics/kgraphviewer/"
    TYPE RECOMMENDED)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_definitions(-Wall
    -DQT_NO_URL_CAST_FROM_STRING
    -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_USE_QSTRINGBUILDER
)

add_subdirectory(massifdata)
add_subdirectory(visualizer)
add_subdirectory(app)
add_subdirectory(pics)

if (BUILD_TESTING)
    add_subdirectory(test)
endif()

ki18n_install(po)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
