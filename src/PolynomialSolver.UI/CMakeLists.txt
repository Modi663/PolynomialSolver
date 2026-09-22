add_library(PolynomialSolver.Plotting STATIC
    PlotDataBuilder.cpp
)

target_include_directories(PolynomialSolver.Plotting
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(PolynomialSolver.Plotting PUBLIC Qt6::Core)
target_compile_features(PolynomialSolver.Plotting PUBLIC cxx_std_20)

if(MINGW)
    target_compile_options(PolynomialSolver.Plotting PRIVATE -Wall -Wextra -Wpedantic)
endif()
