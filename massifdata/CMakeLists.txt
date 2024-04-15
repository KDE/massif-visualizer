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
    Qt6::Core
    Qt6::Core5Compat
    KF6::Archive
    KF6::KIOCore
    KF6::I18n
    KF6::ConfigCore
)
