# Configuration of the static analyzer tool, aka. clang-tidy.
# See https://clang.llvm.org/extra/clang-tidy/

# The use of clang-tidy is optional, but recommended for maintaining code quality.
# Still, in case of issue with this tool, it can be completely disabled without affecting the build process.
# Use `-DHIPOP_USE_CLANG_TIDY=OFF` when calling CMake to disable it.
option(HIPOP_USE_CLANG_TIDY "Enable clang-tidy (if available) while compiling" ON)

# If HIPOP_USE_CLANG_TIDY_MANDATORY is ON, the build will fail if clang-tidy is not found.
# This mechanism is intended to be used in CI/CD pipelines, i.e. in environments where it is important
# to be sure that the quality gates enforced by clang-tidy are respected.
# In a local development environment, this setting can be left to OFF (default value),
# so that clang-tidy is used only if available.
option(HIPOP_USE_CLANG_TIDY_MANDATORY "Make clang-tidy usage mandatory" OFF)

# Stop the configuration early if clang-tidy is disabled through HIPOP_USE_CLANG_TIDY=OFF.
if(NOT HIPOP_USE_CLANG_TIDY)
    if(HIPOP_USE_CLANG_TIDY_MANDATORY)
        message(
            FATAL_ERROR
            "Inconsistent configuration: HIPOP_USE_CLANG_TIDY must be ON if HIPOP_USE_CLANG_TIDY_MANDATORY is ON"
        )
    endif()
    return()
endif()

# Search the `clang-tidy` executable in the system.
# Aside from the standard system paths, this search also considers
# the directory specified by the environment variable `CLANG_TIDY_ROOT`.
find_program(CLANG_TIDY
    NAMES clang-tidy
    HINTS ENV CLANG_TIDY_ROOT
    PATH_SUFFIXES bin
)

if(CLANG_TIDY)

    message(STATUS "Found clang-tidy: static analysis enabled")

    # The configuration of the clang-tidy tool is in the `.clang-tidy` files (base configuration
    # in the repository root, and incremental overrides in subdirectories where needed).
    # Thus, no additional configuration is needed here.
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY}")

elseif(HIPOP_USE_CLANG_TIDY_MANDATORY)
    message(FATAL_ERROR "Cannot find clang-tidy: abort since static analysis is marked as mandatory")
else()
    message(STATUS "Cannot find clang-tidy: static analysis not enabled")
endif()
