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
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Gui
    KF${KF_MAJOR_VERSION}::KIOCore
    KF${KF_MAJOR_VERSION}::I18n
)
if (QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(mv-visualizer LINK_PRIVATE KChart6 Qt6::Core5Compat)
else()
    target_link_libraries(mv-visualizer LINK_PRIVATE KChart)
endif()
