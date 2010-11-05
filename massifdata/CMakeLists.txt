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
)

kde4_add_library(massifdata ${massifdata_SRCS})

target_link_libraries(massifdata
    ${QT_QTCORE_LIBRARY}
)
install(TARGETS massifdata ${INSTALL_TARGETS_DEFAULT_ARGS})