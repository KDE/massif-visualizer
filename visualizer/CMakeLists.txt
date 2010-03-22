include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(visualizer_SRCS
    totalcostmodel.cpp
    detailedcostmodel.cpp
    datatreemodel.cpp
    filtereddatatreemodel.cpp
    dotgraphgenerator.cpp
)

kde4_add_library(visualizer ${visualizer_SRCS})

target_link_libraries(visualizer
    massifdata
    kdchart
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${KDE4_KDECORE_LIBS}
)
