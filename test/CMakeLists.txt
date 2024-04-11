ecm_add_test(datamodeltest datamodeltest.cpp
    TEST_NAME "datamodeltest"
    LINK_LIBRARIES mv-massifdata mv-visualizer Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Test KF${KF_MAJOR_VERSION}::KIOCore KF${KF_MAJOR_VERSION}::ConfigCore
)

add_executable(massifparser massifparser.cpp)

target_link_libraries(massifparser LINK_PRIVATE
    Qt${QT_MAJOR_VERSION}::Core
    KF${KF_MAJOR_VERSION}::Archive
    KF${KF_MAJOR_VERSION}::CoreAddons
    mv-massifdata
)
