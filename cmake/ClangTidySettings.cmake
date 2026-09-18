# Configuration of the static analyzer tool, aka. clang-tidy.
# See https://clang.llvm.org/extra/clang-tidy/

# The use of clang-tidy is disabled by default.
# Use `-DHIPOP_USE_CLANG_TIDY=ON` when calling CMake to enable it.
option(HIPOP_USE_CLANG_TIDY "Enable clang-tidy while compiling" OFF)

if(HIPOP_USE_CLANG_TIDY)

    message(STATUS "Enabling static analysis using clang-tidy")

    # Search the `clang-tidy` executable in the system.
    # Aside from the standard system paths, this search also considers
    # the directory specified by the environment variable `CLANG_TIDY_ROOT`.
    find_program(CLANG_TIDY
        NAMES clang-tidy
        HINTS ENV CLANG_TIDY_ROOT
        PATH_SUFFIXES bin
        REQUIRED
    )

    # The configuration of the clang-tidy tool is in the `.clang-tidy` files (base configuration
    # in the repository root, and incremental overrides in subdirectories where needed).
    # Thus, no additional configuration is needed here.
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY}")

endif()
