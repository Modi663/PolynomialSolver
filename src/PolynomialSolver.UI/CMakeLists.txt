add_library(PolynomialSolver.UI STATIC
    CoefficientParser.cpp
    MainWindow.cpp
    MainWindow.ui
    PlotDataBuilder.cpp
    PolynomialChartView.cpp
    CoefficientParser.h
    MainWindow.h
    PlotDataBuilder.h
    PolynomialChartView.h
)

target_include_directories(PolynomialSolver.UI
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(PolynomialSolver.UI
    PUBLIC
        PolynomialSolver.Core
        Qt6::Charts
        Qt6::Core
        Qt6::Widgets
)
target_compile_features(PolynomialSolver.UI PUBLIC cxx_std_20)

if(MINGW)
    target_compile_options(PolynomialSolver.UI PRIVATE -Wall -Wextra -Wpedantic)
endif()

qt_add_executable(PolynomialSolver WIN32
    main.cpp
    resources.qrc
    windows-icon.rc
)

target_link_libraries(PolynomialSolver PRIVATE PolynomialSolver.UI)

install(TARGETS PolynomialSolver RUNTIME DESTINATION bin)

qt_generate_deploy_app_script(
    TARGET PolynomialSolver
    OUTPUT_SCRIPT deployScript
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deployScript})
