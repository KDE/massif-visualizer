include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(visualizer_SRCS
    costmodel.cpp
)

kde4_add_library(visualizer ${visualizer_SRCS})

target_link_libraries(visualizer
    massifdata
    ${QT_QTCORE_LIBRARY}
)
