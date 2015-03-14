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
    allocatorsmodel.cpp
)

add_library(mv-visualizer STATIC ${visualizer_SRCS})

target_link_libraries(mv-visualizer
LINK_PUBLIC
    KF5::KDELibs4Support
LINK_PRIVATE
    mv-massifdata
    mv-kdchart
    Qt5::Core
    Qt5::Gui
    KF5::KIOCore
    KF5::I18n
)
