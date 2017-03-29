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

ki18n_wrap_ui(massif-visualizer_SRCS mainwindow.ui config.ui)

add_executable(massif-visualizer ${massif-visualizer_SRCS})

target_link_libraries(massif-visualizer LINK_PRIVATE
    Qt5::PrintSupport
    Qt5::Svg
    KF5::KIOCore
    KF5::Parts
    KF5::I18n
    KChart
    mv-massifdata
    mv-visualizer
)

install( TARGETS massif-visualizer  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(
    FILES massif-visualizerui.rc documentwidgetui.rc charttabui.rc callgraphtabui.rc
    DESTINATION ${KXMLGUI_INSTALL_DIR}/massif-visualizer
)

install(FILES massif-visualizer-settings.kcfg DESTINATION ${KCFG_INSTALL_DIR})

install(PROGRAMS org.kde.massif-visualizer.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES org.kde.massif-visualizer.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

# XML mime type
set( SHARED_MIME_INFO_MINIMUM_VERSION "0.30" )
set( XDG_MIME_INSTALL_DIR "share/mime/packages" )
find_package( SharedMimeInfo )
if(SHAREDMIMEINFO_FOUND)
    install( FILES massif.xml DESTINATION ${XDG_MIME_INSTALL_DIR} )
    update_xdg_mimetypes( ${XDG_MIME_INSTALL_DIR} )
endif()
