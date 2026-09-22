add_library(PolynomialSolver.Core STATIC
    CubicSolver.cpp
    LinearSolver.cpp
    Polynomial.cpp
    QuadraticSolver.cpp
    RootVerifier.cpp
)

target_include_directories(PolynomialSolver.Core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(PolynomialSolver.Core PUBLIC cxx_std_20)

if(MINGW)
    target_compile_options(PolynomialSolver.Core PRIVATE -Wall -Wextra -Wpedantic)
endif()
