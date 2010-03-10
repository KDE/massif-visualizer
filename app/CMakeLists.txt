include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
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
    ${KDE4_THREADWEAVER_LIBRARIES}
    kdchart
    massifdata
    visualizer
)

install( FILES massif-visualizerui.rc DESTINATION ${DATA_INSTALL_DIR}/massif-visualizer )