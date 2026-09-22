function(add_core_test target source)
    qt_add_executable(${target} ${source})
    target_link_libraries(${target}
        PRIVATE
            PolynomialSolver.Core
            Qt6::Test
    )
    add_test(NAME ${target} COMMAND ${target})
    set_tests_properties(${target} PROPERTIES
        ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
endfunction()

add_core_test(PolynomialTests PolynomialTests.cpp)
add_core_test(LinearSolverTests LinearSolverTests.cpp)
add_core_test(QuadraticSolverTests QuadraticSolverTests.cpp)
add_core_test(CubicSolverTests CubicSolverTests.cpp)
add_core_test(RootVerifierTests RootVerifierTests.cpp)

qt_add_executable(SolvePolynomialUseCaseTests SolvePolynomialUseCaseTests.cpp)
target_link_libraries(SolvePolynomialUseCaseTests
    PRIVATE
        PolynomialSolver.Application
        Qt6::Test
)
add_test(NAME SolvePolynomialUseCaseTests COMMAND SolvePolynomialUseCaseTests)
set_tests_properties(SolvePolynomialUseCaseTests PROPERTIES
    ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
)

qt_add_executable(PlotDataBuilderTests PlotDataBuilderTests.cpp)
target_link_libraries(PlotDataBuilderTests
    PRIVATE
        PolynomialSolver.Plotting
        Qt6::Test
)
add_test(NAME PlotDataBuilderTests COMMAND PlotDataBuilderTests)
set_tests_properties(PlotDataBuilderTests PROPERTIES
    ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
)
