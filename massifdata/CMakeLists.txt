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

target_link_libraries(mv-massifdata LINK_PRIVATE
    Qt5::Core
    KF5::Archive
    KF5::KIOCore
    KF5::I18n
    KF5::KDELibs4Support
)
