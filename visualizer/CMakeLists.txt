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
LINK_PRIVATE
    mv-massifdata
    Qt6::Core
    Qt6::Core5Compat
    Qt6::Gui
    KF6::KIOCore
    KF6::I18n
    KChart6
)
