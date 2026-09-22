add_library(PolynomialSolver.UI STATIC
    CoefficientParser.cpp
    MainWindow.cpp
    PlotDataBuilder.cpp
    PolynomialChartView.cpp
)

target_include_directories(PolynomialSolver.UI
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(PolynomialSolver.UI
    PUBLIC
        PolynomialSolver.Application
        Qt6::Core
        Qt6::Widgets
)
target_compile_features(PolynomialSolver.UI PUBLIC cxx_std_20)

if(MINGW)
    target_compile_options(PolynomialSolver.UI PRIVATE -Wall -Wextra -Wpedantic)
endif()
