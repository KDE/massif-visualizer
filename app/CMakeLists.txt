find_path(KGRAPHVIEWER_INCLUDE_PATH "kgraphviewer/kgraphviewer_interface.h")

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${KGRAPHVIEWER_INCLUDE_PATH}
)

set(massif-visualizer_SRCS
    main.cpp
    mainwindow.cpp
)

kde4_add_ui_files(massif-visualizer_SRCS mainwindow.ui)

kde4_add_executable(massif-visualizer ${massif-visualizer_SRCS})

target_link_libraries(massif-visualizer
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDE4_KPARTS_LIBS}
    kdchart
    massifdata
    visualizer
)

install( TARGETS massif-visualizer  ${INSTALL_TARGETS_DEFAULT_ARGS} )

install( FILES massif-visualizerui.rc DESTINATION ${DATA_INSTALL_DIR}/massif-visualizer )