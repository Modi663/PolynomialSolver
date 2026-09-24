cmake_minimum_required(VERSION 3.25)

project(PolynomialSolver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

include(CTest)

find_package(Qt6 6.11 REQUIRED COMPONENTS Charts Test Widgets)

add_subdirectory(src/PolynomialSolver.Core)
add_subdirectory(src/PolynomialSolver.UI)

if(BUILD_TESTING)
    add_subdirectory(tests/QtTests)
endif()
