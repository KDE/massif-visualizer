ecm_add_test(datamodeltest datamodeltest.cpp
    TEST_NAME "datamodeltest"
    LINK_LIBRARIES mv-massifdata mv-visualizer Qt6::Core Qt6::Test KF6::KIOCore KF6::ConfigCore
)

add_executable(massifparser massifparser.cpp)

target_link_libraries(massifparser
PRIVATE
    Qt6::Core
    KF6::Archive
    KF6::CoreAddons
    mv-massifdata
)
