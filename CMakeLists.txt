cmake_minimum_required(VERSION 3.16)

# Release Service version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "25")
set (RELEASE_SERVICE_VERSION_MINOR "07")
set (RELEASE_SERVICE_VERSION_MICRO "80")
set (RELEASE_SERVICE_COMPACT_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}${RELEASE_SERVICE_VERSION_MINOR}${RELEASE_SERVICE_VERSION_MICRO}")

# Bump MASSIF_VISUALIZER_BASE_VERSION once new features are added
set(MASSIF_VISUALIZER_BASE_VERSION "0.8")
set(MASSIF_VISUALIZER_VERSION "${MASSIF_VISUALIZER_BASE_VERSION}.${RELEASE_SERVICE_COMPACT_VERSION}")

project(massif-visualizer VERSION ${MASSIF_VISUALIZER_VERSION})

set(QT_MIN_VERSION "6.5.0")
set(KF_MIN_VERSION "6.0.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)

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
include(ECMDeprecationSettings)
include(FeatureSummary)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS
    Core
    Core5Compat
    Widgets
    Svg
    Test
    PrintSupport
)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Archive
    Config
    CoreAddons
    Parts
    KIO
    I18n
)

find_package(KChart6 "3.0.0" REQUIRED)

find_package(KGraphViewerPart 2.5.0)
set_package_properties(KGraphViewerPart PROPERTIES
    DESCRIPTION "KPart to view callgraph dot files"
    PURPOSE "Required for detailed snapshot analysis"
    URL "https://commits.kde.org/kgraphviewer"
    TYPE RECOMMENDED)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# added by KDE_COMPILERSETTINGS_LEVEL >= 5.85.0
remove_definitions(
    -DQT_NO_KEYWORDS
    -DQT_NO_FOREACH
)

ecm_set_disabled_deprecation_versions(
    QT 6.5
    KF 6.0
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
