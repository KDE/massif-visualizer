include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(massifdata_SRCS
    filedata.cpp
    snapshotitem.cpp
    treeleafitem.cpp
    parser.cpp
    parserprivate.cpp
    parseworker.cpp
    util.cpp
)

add_library(mv-massifdata STATIC ${massifdata_SRCS})

target_link_libraries(mv-massifdata
LINK_PRIVATE
    Qt${QT_MAJOR_VERSION}::Core
    KF${KF_MAJOR_VERSION}::Archive
    KF${KF_MAJOR_VERSION}::KIOCore
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::ConfigCore
)

if (QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(mv-massifdata LINK_PRIVATE Qt6::Core5Compat)
endif()
