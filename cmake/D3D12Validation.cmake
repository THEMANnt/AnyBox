set(ANYBOX_DXGI_DEBUG_LIBRARY "" CACHE FILEPATH "Application-local Windows DXGI debug library")
if(ANYBOX_DXGI_DEBUG_LIBRARY)
    if(NOT EXISTS "${ANYBOX_DXGI_DEBUG_LIBRARY}" OR IS_DIRECTORY "${ANYBOX_DXGI_DEBUG_LIBRARY}")
        message(FATAL_ERROR "ANYBOX_DXGI_DEBUG_LIBRARY must name an existing DXGIDebug.dll")
    endif()
    add_custom_target(anybox_d3d12_debug_runtime
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${ANYBOX_RUNTIME_DIRECTORY}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${ANYBOX_DXGI_DEBUG_LIBRARY}" "${ANYBOX_RUNTIME_DIRECTORY}/DXGIDebug.dll"
        BYPRODUCTS "${ANYBOX_RUNTIME_DIRECTORY}/DXGIDebug.dll"
        VERBATIM
    )
    add_dependencies(d3d12_device_tests anybox_d3d12_debug_runtime)
endif()
