# Configuration of interprocedural optimization (IPO), aka. link-time optimization (LTO).

# The use of IPO/LTO is disabled by default.
# Use `-DHIPOP_USE_IPO_LTO=ON` when calling CMake to enable it.
option(HIPOP_USE_IPO_LTO "Enable interprocedural optimization (IPO/LTO)" OFF)

# Warning: when IPO/LTO is enabled, static libraries contain compiler-specific bytecode instead of machine code,
# thus they cannot be linked by consumers built without IPO/LTO or with another compiler (or compiler version).
# Therefore, IPO/LTO should not be enabled when building static libraries meant to be distributed.
# Executables and shared libraries are not affected, as IPO/LTO is fully resolved when they are linked.

set(HIPOP_IS_IPO_LTO_ENABLED OFF)
if(HIPOP_USE_IPO_LTO)

    include(CheckIPOSupported)
    check_ipo_supported(RESULT HIPOP_IS_IPO_LTO_SUPPORTED)

    if(HIPOP_IS_IPO_LTO_SUPPORTED)
        message(STATUS "Enabling IPO/LTO for non-debug configurations")
        set(HIPOP_IS_IPO_LTO_ENABLED ON)
    else()
        message(WARNING "IPO/LTO is requested but not supported by the compiler, thus it is disabled")
    endif()
endif()

# Enable IPO/LTO for the given target if `HIPOP_USE_IPO_LTO` is ON and if the compiler supports it.
function(hipop_enable_ipo_lto_if_requested target)
    if(HIPOP_IS_IPO_LTO_ENABLED)

        # Interprocedural optimization is not suitable in debug mode (it makes linking slower and debugging harder),
        # hence it is enabled only for the optimized configurations.
        set_target_properties(${target} PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
            INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON
            INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON
        )
    endif()
endfunction()
