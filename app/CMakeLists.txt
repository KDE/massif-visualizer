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

qt5_add_resources(massif-visualizer_SRCS massif-visualizer.qrc)

if(KGraphViewerPart_FOUND)
    add_definitions(-DHAVE_KGRAPHVIEWER)
    list(APPEND massif-visualizer_SRCS callgraphtab.cpp)
    qt5_add_resources(massif-visualizer_SRCS callgraphtab.qrc)
endif()

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

if (TARGET KGraphViewerPart)
    target_link_libraries(massif-visualizer PRIVATE KGraphViewerPart)
elseif(KGraphViewerPart_FOUND)
    include_directories(
        ${KGraphViewerPart_INCLUDE_DIRS}
    )
endif()

install( TARGETS massif-visualizer  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES massif-visualizer-settings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})

install(PROGRAMS org.kde.massif-visualizer.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.massif-visualizer.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

# XML mime type
set( SHARED_MIME_INFO_MINIMUM_VERSION "0.30" )
find_package( SharedMimeInfo ${SHARED_MIME_INFO_MINIMUM_VERSION})
if(SHAREDMIMEINFO_FOUND)
    install( FILES massif.xml DESTINATION ${KDE_INSTALL_MIMEDIR} )
    update_xdg_mimetypes( ${KDE_INSTALL_MIMEDIR} )
endif()
