PROJECT(massif-visualizer)

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ ${CMAKE_MODULE_PATH})

find_package(KDE4 REQUIRED)
find_package(KGraphViewer REQUIRED)

include_directories(
    ${KDE4_INCLUDES}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/kdchart/include
    ${CMAKE_CURRENT_SOURCE_DIR}/kdchart/src
    ${CMAKE_CURRENT_BINARY_DIR}/kdchart/src/
)

add_definitions(-Wall)

add_subdirectory(kdchart)
add_subdirectory(massifdata)
add_subdirectory(visualizer)
add_subdirectory(app)

add_subdirectory(test)
