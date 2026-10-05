set(HIPOP_HEADERS
    include/hipop/create.h
    include/hipop/graph.h
    include/hipop/shortest_path.h
)

set(HIPOP_SRC
    src/create.cpp
    src/graph.cpp
    src/shortest_path.cpp
)

add_library(hipoplib ${HIPOP_SRC})
set_target_properties(hipoplib PROPERTIES
    POSITION_INDEPENDENT_CODE ON # Needed to link the hipop static lib to the pybind dynamic one
)
target_include_directories(hipoplib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(hipoplib PRIVATE OpenMP::OpenMP_CXX)
hipop_configure_target(hipoplib)


# ---------------------------------
# Tests and examples (optional)

if(HIPOP_WITH_TESTS)
    message(STATUS "Building HiPOP tests (for core library)")
    add_subdirectory(tests)
endif()

if(HIPOP_WITH_EXAMPLES)
    message(STATUS "Building HiPOP examples (for core library)")
    add_subdirectory(examples)
endif()


# ---------------------------------
# Install (core library only)

# Disable deployment of the core library artifacts when building the Python wheel for the wrapper:
# the core library is statically linked into the .so/.dylib/.dll file implementing the Python extension,
# thus they don't need to be included in the wheel.
if(NOT DEFINED SKBUILD)

    include(CMakePackageConfigHelpers)
    include(GNUInstallDirs)

    install(
        DIRECTORY include/ # Trailing slash means "copy the content of this directory", do not remove it!
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
        PATTERN "*_p.h" EXCLUDE
    )

    install(
        TARGETS hipoplib
        EXPORT HipopTarget
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT RuntimeExecutables
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT RuntimeLibraries
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Development
    )

    install(EXPORT HipopTarget DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hipop)

    write_basic_package_version_file(
        ${CMAKE_BINARY_DIR}/Hipop/HipopConfigVersion.cmake
        VERSION ${HiPOP_VERSION}
        COMPATIBILITY ExactVersion
    )

    configure_package_config_file(
        cmake/HipopConfig.cmake.in
        ${CMAKE_BINARY_DIR}/Hipop/HipopConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hipop
    )

    install(
        FILES
            ${CMAKE_BINARY_DIR}/Hipop/HipopConfig.cmake
            ${CMAKE_BINARY_DIR}/Hipop/HipopConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hipop
        COMPONENT Development
    )

endif()
