include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../kdablibfakes/include
    ${CMAKE_CURRENT_SOURCE_DIR}/KDChart
    ${CMAKE_CURRENT_SOURCE_DIR}/KDChart/Cartesian
    ${CMAKE_CURRENT_SOURCE_DIR}/KDChart/Cartesian/DiagramFlavors
    ${CMAKE_CURRENT_SOURCE_DIR}/KDChart/Polar
    ${CMAKE_CURRENT_SOURCE_DIR}/KDChart/Ternary
)

ADD_DEFINITIONS(-DKDCHART_BUILD_KDCHART_LIB)

########### next target ###############

set(kdchart_LIB_SRCS
    KDChart/Cartesian/DiagramFlavors/KDChartNormalBarDiagram_p.cpp
    KDChart/Cartesian/DiagramFlavors/KDChartNormalLineDiagram_p.cpp
    KDChart/Cartesian/DiagramFlavors/KDChartNormalLyingBarDiagram_p.cpp
    KDChart/Cartesian/DiagramFlavors/KDChartNormalPlotter_p.cpp
    KDChart/Cartesian/DiagramFlavors/KDChartPercentBarDiagram_p.cpp
    KDChart/Cartesian/DiagramFlavors/KDChartPercentLineDiagram_p.cpp
    KDChart/Cartesian/DiagramFlavors/KDChartPercentLyingBarDiagram_p.cpp
    KDChart/Cartesian/DiagramFlavors/KDChartPercentPlotter_p.cpp
    KDChart/Cartesian/DiagramFlavors/KDChartStackedPlotter_p.cpp
    KDChart/Cartesian/DiagramFlavors/KDChartStackedBarDiagram_p.cpp
    KDChart/Cartesian/DiagramFlavors/KDChartStackedLineDiagram_p.cpp
    KDChart/Cartesian/DiagramFlavors/KDChartStackedLyingBarDiagram_p.cpp
    KDChart/Cartesian/KDChartAbstractCartesianDiagram.cpp
    KDChart/Cartesian/KDChartBarAttributes.cpp
    KDChart/Cartesian/KDChartBarDiagram.cpp
    KDChart/Cartesian/KDChartBarDiagram_p.cpp
    KDChart/Cartesian/KDChartCartesianAxis.cpp
    KDChart/Cartesian/KDChartCartesianCoordinatePlane.cpp
    KDChart/Cartesian/KDChartCartesianDiagramDataCompressor_p.cpp
    KDChart/Cartesian/KDChartCartesianGrid.cpp
    KDChart/Cartesian/KDChartLeveyJenningsAxis.cpp
    KDChart/Cartesian/KDChartLeveyJenningsCoordinatePlane.cpp
    KDChart/Cartesian/KDChartLeveyJenningsDiagram.cpp
    KDChart/Cartesian/KDChartLeveyJenningsDiagram_p.cpp
    KDChart/Cartesian/KDChartLeveyJenningsGridAttributes.cpp
    KDChart/Cartesian/KDChartLeveyJenningsGrid.cpp
    KDChart/Cartesian/KDChartLineDiagram.cpp
    KDChart/Cartesian/KDChartLineDiagram_p.cpp
    KDChart/Cartesian/KDChartPlotter.cpp
    KDChart/Cartesian/KDChartPlotterDiagramCompressor.cpp
    KDChart/Cartesian/KDChartPlotter_p.cpp
    KDChart/Cartesian/KDChartStockBarAttributes.cpp
    KDChart/Cartesian/KDChartStockDiagram.cpp
    KDChart/Cartesian/KDChartStockDiagram_p.cpp
    KDChart/Cartesian/KDChartThreeDBarAttributes.cpp
    KDChart/Cartesian/PaintingHelpers_p.cpp
    KDChart/ChartGraphicsItem.cpp
    KDChart/KDChartAbstractAreaBase.cpp
    KDChart/KDChartAbstractArea.cpp
    KDChart/KDChartAbstractAreaWidget.cpp
    KDChart/KDChartAbstractAxis.cpp
    KDChart/KDChartAbstractCoordinatePlane.cpp
    KDChart/KDChartAbstractDiagram.cpp
    KDChart/KDChartAbstractDiagram_p.cpp
    KDChart/KDChartAbstractGrid.cpp
    KDChart/KDChartAbstractProxyModel.cpp
    KDChart/KDChartAbstractThreeDAttributes.cpp
    KDChart/KDChartAttributesModel.cpp
    KDChart/KDChartBackgroundAttributes.cpp
    KDChart/KDChartChart.cpp
    KDChart/KDChartDatasetProxyModel.cpp
    KDChart/KDChartDatasetSelector.cpp
    KDChart/KDChartDataValueAttributes.cpp
    KDChart/KDChartDiagramObserver.cpp
    KDChart/KDChartFrameAttributes.cpp
    KDChart/KDChartGridAttributes.cpp
    KDChart/KDChartHeaderFooter.cpp
    KDChart/KDChartLayoutItems.cpp
    KDChart/KDChartLegend.cpp
    KDChart/KDChartLineAttributes.cpp
    KDChart/KDChartMarkerAttributes.cpp
    KDChart/KDChartMeasure.cpp
    KDChart/KDChartModelDataCache_p.cpp
    KDChart/KDChartPaintContext.cpp
    KDChart/KDChartPalette.cpp
    KDChart/KDChartPosition.cpp
    KDChart/KDChartPrintingParameters.cpp
    KDChart/KDChartRelativePosition.cpp
    KDChart/KDChartRulerAttributes.cpp
    KDChart/KDChartSignalCompressor.cpp
    KDChart/KDChartTextArea.cpp
    KDChart/KDChartTextAttributes.cpp
    KDChart/KDChartTextLabelCache.cpp
    KDChart/KDChartThreeDLineAttributes.cpp
    KDChart/KDChartValueTrackerAttributes.cpp
    KDChart/KDChartWidget.cpp
    KDChart/KDTextDocument.cpp
    KDChart/Polar/KDChartAbstractPieDiagram.cpp
    KDChart/Polar/KDChartAbstractPolarDiagram.cpp
    KDChart/Polar/KDChartPieAttributes.cpp
    KDChart/Polar/KDChartPieDiagram.cpp
    KDChart/Polar/KDChartPolarCoordinatePlane.cpp
    KDChart/Polar/KDChartPolarDiagram.cpp
    KDChart/Polar/KDChartPolarGrid.cpp
    KDChart/Polar/KDChartRadarCoordinatePlane.cpp
    KDChart/Polar/KDChartRadarDiagram.cpp
    KDChart/Polar/KDChartRadarGrid.cpp
    KDChart/Polar/KDChartRingDiagram.cpp
    KDChart/Polar/KDChartThreeDPieAttributes.cpp
    KDChart/ReverseMapper.cpp
    KDChart/Ternary/KDChartAbstractTernaryDiagram.cpp
    KDChart/Ternary/KDChartTernaryAxis.cpp
    KDChart/Ternary/KDChartTernaryCoordinatePlane.cpp
    KDChart/Ternary/KDChartTernaryGrid.cpp
    KDChart/Ternary/KDChartTernaryLineDiagram.cpp
    KDChart/Ternary/KDChartTernaryPointDiagram.cpp
    KDChart/Ternary/TernaryConstants.cpp
    KDChart/Ternary/TernaryPoint.cpp
)

qt4_wrap_ui(kdchart_LIB_SRCS
    KDChart/KDChartDatasetSelector.ui
)

qt_wrap_cpp(mv-kdchart kdchart_LIB_SRCS KDChart/KDChartEnums.h)

add_library(mv-kdchart STATIC ${kdchart_LIB_SRCS})

target_link_libraries(mv-kdchart LINK_PRIVATE
${QT_QTSVG_LIBRARY} ${QT_QTXML_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTCORE_LIBRARY} ${ZLIB_LIBRARY} )
