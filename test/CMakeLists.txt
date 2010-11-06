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