pybind11_add_module(cpp
    hipop/cpp/main.cpp
    hipop/cpp/graph.cpp
    hipop/cpp/shortest_path.cpp
)
target_link_libraries(cpp PRIVATE hipoplib)

# Only install the extension (and its type stubs) when the build is driven by scikit-build-core.
if(DEFINED SKBUILD)
    install(TARGETS cpp LIBRARY DESTINATION hipop)

    # Generate the type stubs of the installed extension, and add them to the installation.
    set(HIPOP_STUBS_DIR "${CMAKE_CURRENT_BINARY_DIR}/stubs")
    configure_file(cmake/GenerateStubs.cmake.in GenerateStubs.cmake @ONLY)
    install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/GenerateStubs.cmake")
    install(DIRECTORY "${HIPOP_STUBS_DIR}/" DESTINATION .)

endif()
