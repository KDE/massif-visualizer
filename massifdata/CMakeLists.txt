add_library(mv-massifdata STATIC)

target_sources(mv-massifdata PRIVATE
    filedata.cpp
    snapshotitem.cpp
    treeleafitem.cpp
    parser.cpp
    parserprivate.cpp
    parseworker.cpp
    util.cpp
)

target_link_libraries(mv-massifdata
PRIVATE
    Qt6::Core
    Qt6::Core5Compat
    KF6::Archive
    KF6::KIOCore
    KF6::I18n
    KF6::ConfigCore
)
