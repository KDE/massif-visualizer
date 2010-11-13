include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(KGRAPHVIEWER_FOUND)
    include_directories(
        ${KGRAPHVIEWER_INCLUDE_DIRECTORIES}
    )
    add_definitions(-DHAVE_KGRAPHVIEWER)
endif(KGRAPHVIEWER_FOUND)

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
    mv-kdchart
    mv-massifdata
    mv-visualizer
)

install( TARGETS massif-visualizer  ${INSTALL_TARGETS_DEFAULT_ARGS} )

install( FILES massif-visualizerui.rc DESTINATION ${DATA_INSTALL_DIR}/massif-visualizer )

# XML mime type
set( SHARED_MIME_INFO_MINIMUM_VERSION "0.30" )
set( XDG_MIME_INSTALL_DIR "share/mime/packages" )
find_package( SharedMimeInfo )
if( SHARED_MIME_INFO_FOUND )
    install( FILES massif.xml DESTINATION ${XDG_MIME_INSTALL_DIR} )
    update_xdg_mimetypes( ${XDG_MIME_INSTALL_DIR} )
endif( SHARED_MIME_INFO_FOUND )
