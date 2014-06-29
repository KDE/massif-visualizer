set(datamodeltest_SRCS
    datamodeltest.cpp
    modeltest.cpp
)
kde4_add_unit_test(datamodeltest ${datamodeltest_SRCS})
target_link_libraries(datamodeltest LINK_PRIVATE
    mv-massifdata
    mv-visualizer
    ${QT_QTTEST_LIBRARY}
)

kde4_add_executable(massifparser massifparser.cpp)

target_link_libraries(massifparser LINK_PRIVATE
    ${QT_QTCORE_LIBRARY}
    ${KDE4_KDECORE_LIBS}
    mv-massifdata
)
