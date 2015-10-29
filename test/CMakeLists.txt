ecm_add_test(datamodeltest datamodeltest.cpp modeltest.cpp
    TEST_NAME "datamodeltest"
    LINK_LIBRARIES mv-massifdata mv-visualizer Qt5::Core Qt5::Test KF5::KIOCore
)

add_executable(massifparser massifparser.cpp)

target_link_libraries(massifparser LINK_PRIVATE
    Qt5::Core
    KF5::Archive
    KF5::CoreAddons
    mv-massifdata
)
