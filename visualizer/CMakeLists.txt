add_library(mv-visualizer STATIC)

target_sources(mv-visualizer PRIVATE
    totalcostmodel.cpp
    detailedcostmodel.cpp
    datatreemodel.cpp
    filtereddatatreemodel.cpp
    dotgraphgenerator.cpp
    allocatorsmodel.cpp
)

target_link_libraries(mv-visualizer
PRIVATE
    mv-massifdata
    Qt6::Core
    Qt6::Core5Compat
    Qt6::Gui
    KF6::KIOCore
    KF6::I18n
    KChart6
)
