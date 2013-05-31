include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(massifdata_SRCS
    filedata.cpp
    snapshotitem.cpp
    treeleafitem.cpp
    parser.cpp
    parserbase.cpp
    massifparser.cpp
    parseworker.cpp
    util.cpp
)

kde4_add_library(mv-massifdata ${massifdata_SRCS})

target_link_libraries(mv-massifdata
    ${QT_QTCORE_LIBRARY}
    # required for Qt::escape() in QTextDocument
    ${QT_QTGUI_LIBRARY}
)
