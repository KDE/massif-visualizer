include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(massif-visualizer_SRCS
    main.cpp
    mainwindow.cpp
    configdialog.cpp
    documentwidget.cpp
    documenttabinterface.cpp
    charttab.cpp
    allocatorstab.cpp
)

if(KGRAPHVIEWER_FOUND)
    include_directories(
        ${KGRAPHVIEWER_INCLUDE_DIRECTORIES}
    )
    add_definitions(-DHAVE_KGRAPHVIEWER)
    list(APPEND massif-visualizer_SRCS callgraphtab.cpp)
endif(KGRAPHVIEWER_FOUND)

kconfig_add_kcfg_files(massif-visualizer_SRCS massif-visualizer-settings.kcfgc)

qt5_wrap_ui(massif-visualizer_SRCS mainwindow.ui config.ui)

add_executable(massif-visualizer ${massif-visualizer_SRCS})

target_link_libraries(massif-visualizer LINK_PRIVATE
    Qt5::PrintSupport
    KF5::KIOCore
    KF5::KDELibs4Support
    KF5::Parts
    KF5::I18n
    mv-kdchart
    mv-massifdata
    mv-visualizer
)

install( TARGETS massif-visualizer  ${INSTALL_TARGETS_DEFAULT_ARGS} )

install(
    FILES massif-visualizerui.rc documentwidgetui.rc charttabui.rc callgraphtabui.rc
    DESTINATION ${DATA_INSTALL_DIR}/massif-visualizer
)

install(FILES massif-visualizer-settings.kcfg DESTINATION ${KCFG_INSTALL_DIR})

install(PROGRAMS massif-visualizer.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})

# XML mime type
set( SHARED_MIME_INFO_MINIMUM_VERSION "0.30" )
set( XDG_MIME_INSTALL_DIR "share/mime/packages" )
find_package( SharedMimeInfo )
if( SHARED_MIME_INFO_FOUND )
    install( FILES massif.xml DESTINATION ${XDG_MIME_INSTALL_DIR} )
    update_xdg_mimetypes( ${XDG_MIME_INSTALL_DIR} )
endif( SHARED_MIME_INFO_FOUND )
