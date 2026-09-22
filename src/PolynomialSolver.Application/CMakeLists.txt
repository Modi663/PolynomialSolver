add_library(PolynomialSolver.Application STATIC
    SolvePolynomialUseCase.cpp
)

target_include_directories(PolynomialSolver.Application
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(PolynomialSolver.Application
    PUBLIC
        PolynomialSolver.Core
)

target_compile_features(PolynomialSolver.Application PUBLIC cxx_std_20)

if(MINGW)
    target_compile_options(PolynomialSolver.Application PRIVATE -Wall -Wextra -Wpedantic)
endif()
