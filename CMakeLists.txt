PROJECT(massif-visualizer)

enable_testing()

find_package(KDE4 REQUIRED)

include_directories(
    ${KDE4_INCLUDES}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/kdchart/include
    ${CMAKE_CURRENT_SOURCE_DIR}/kdchart/src
    ${CMAKE_CURRENT_BINARY_DIR}/kdchart/src/
)

add_subdirectory(kdchart)
add_subdirectory(model)
add_subdirectory(app)
