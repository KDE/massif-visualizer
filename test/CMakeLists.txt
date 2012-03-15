set(datamodeltest_SRCS
    datamodeltest.cpp
    modeltest.cpp
)
kde4_add_unit_test(datamodeltest ${datamodeltest_SRCS})
target_link_libraries(datamodeltest
    mv-massifdata
    mv-visualizer
    ${QT_QTTEST_LIBRARY}
)

kde4_add_executable(massifparser massifparser.cpp)

target_link_libraries(massifparser
    ${QT_QTCORE_LIBRARY}
    ${KDE4_KDECORE_LIBS}
#     ${KDE4_KDEUI_LIBS}
#     ${KDE4_KIO_LIBS}
#     ${KDE4_KPARTS_LIBS}
#     mv-kdchart
    mv-massifdata
#     mv-visualizer
)