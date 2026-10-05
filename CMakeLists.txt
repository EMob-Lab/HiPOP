cmake_minimum_required(VERSION 3.24)

file(STRINGS VERSION.txt HIPOP_VERSION)
project(HiPOP
    VERSION "${HIPOP_VERSION}"
    DESCRIPTION "High Performance Optimal Path module"
    HOMEPAGE_URL "https://github.com/EMob-Lab/HiPOP"
    LANGUAGES CXX
)

option(HIPOP_WITH_PYTHON "Compile Python wrapper" OFF) # Disabled by default, so that pybind11 is not required then.
option(HIPOP_WITH_TESTS "Compile HiPOP tests (for core library)" ON)
option(HIPOP_WITH_EXAMPLES "Compile HiPOP examples (for core library)" ON)

# Miscellaneous CMake and compiler settings.
include(cmake/MainSettings.cmake)

# 3rd-party dependencies.
find_package(OpenMP REQUIRED)
if(HIPOP_WITH_PYTHON)
    set(PYBIND11_FINDPYTHON ON) # See: https://pybind11.readthedocs.io/en/stable/cmake/#modes
    find_package(pybind11 REQUIRED)
endif()

# Enable CTest and related testing features if requested
# (must be done before adding the subdirectories).
if(HIPOP_WITH_TESTS)
    enable_testing()
endif()

# `cpp` subdirectory -> C++ library with related tests and examples, excluding the Python wrapper.
message(STATUS "Building HiPOP core library")
add_subdirectory(cpp)

# `python` subdirectory -> Python wrapper (the tests and examples specific to the Python wrapper
# are pure Python scripts, thus no compilation is needed for them).
if(HIPOP_WITH_PYTHON)
    message(STATUS "Building HiPOP Python wrapper")
    add_subdirectory(python)
endif()
