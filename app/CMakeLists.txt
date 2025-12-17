add_executable(massif-visualizer)

target_sources(massif-visualizer PRIVATE
    main.cpp
    mainwindow.cpp
    configdialog.cpp
    documentwidget.cpp
    documenttabinterface.cpp
    charttab.cpp
    allocatorstab.cpp
    massif-visualizer.qrc
)

set_source_files_properties(main.cpp PROPERTIES
    COMPILE_DEFINITIONS PROJECT_VERSION="${PROJECT_VERSION}"
)

if(KGraphViewerPart_FOUND)
    target_compile_definitions(massif-visualizer PRIVATE HAVE_KGRAPHVIEWER)
    target_sources(massif-visualizer PRIVATE callgraphtab.cpp callgraphtab.qrc)
endif()

kconfig_add_kcfg_files(massif-visualizer massif-visualizer-settings.kcfgc)

ki18n_wrap_ui(massif-visualizer
    mainwindow.ui
    config.ui
)

target_link_libraries(massif-visualizer PRIVATE
    Qt6::PrintSupport
    Qt6::Svg
    KF6::Completion
    KF6::KIOCore
    KF6::Parts
    KF6::I18n
    mv-massifdata
    mv-visualizer
    KChart6
)

if (TARGET KGraphViewerPart)
    target_link_libraries(massif-visualizer PRIVATE KGraphViewerPart)
endif()

install( TARGETS massif-visualizer  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(FILES massif-visualizer-settings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})

install(PROGRAMS org.kde.massif-visualizer.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.massif_visualizer.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

# XML mime type
set( SHARED_MIME_INFO_MINIMUM_VERSION "0.30" )
find_package( SharedMimeInfo ${SHARED_MIME_INFO_MINIMUM_VERSION})
if(SHAREDMIMEINFO_FOUND)
    install( FILES massif.xml DESTINATION ${KDE_INSTALL_MIMEDIR} )
    update_xdg_mimetypes( ${KDE_INSTALL_MIMEDIR} )
endif()
