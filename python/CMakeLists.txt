nanobind_add_module(cpp
    NB_SUPPRESS_WARNINGS
    hipop/cpp/main.cpp
    hipop/cpp/graph.cpp
    hipop/cpp/shortest_path.cpp
)
target_link_libraries(cpp PRIVATE hipoplib)
hipop_configure_target(cpp)

# Only generate the type stubs and install the extension when the build is driven by scikit-build-core.
if(DEFINED SKBUILD)

    install(TARGETS cpp LIBRARY DESTINATION hipop)

    # First step: stubs are generated in the build directory.
    # It would be better to execute this step at build time, but it would conflict
    # with how scikit-build-core workflow manages editable installs: its import hook
    # redirects `import hipop.cpp` and similar statements to the location on disk
    # where the C++ extension is to be installed - which is not the build directory -
    # and that location is not up-to-date (or may not even exist) at build time.
    install(CODE "file(REMOVE_RECURSE \"${CMAKE_CURRENT_BINARY_DIR}/stubs\")")
    nanobind_add_stub(cpp_stubs
        INSTALL_TIME
        RECURSIVE
        MODULE hipop.cpp
        OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/stubs/hipop"
    )

    # Second step: the generated stubs are installed in the package directory.
    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/stubs/hipop/" DESTINATION hipop)

endif()
