function(anybox_add_addrlib)
    set(addrRoot "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../3rdparty/mesa/src/amd/addrlib")
    set(addrSources
        src/addrinterface.cpp
        src/core/addrelemlib.cpp
        src/core/addrlib.cpp
        src/core/addrlib1.cpp
        src/core/addrlib2.cpp
        src/core/addrlib3.cpp
        src/core/addrobject.cpp
        src/core/addrswizzler.cpp
        src/core/coord.cpp
        src/gfx9/gfx9addrlib.cpp
        src/gfx10/gfx10addrlib.cpp
        src/gfx11/gfx11addrlib.cpp
        src/gfx12/gfx12addrlib.cpp
        src/r800/ciaddrlib.cpp
        src/r800/egbaddrlib.cpp
        src/r800/siaddrlib.cpp
    )
    list(TRANSFORM addrSources PREPEND "${addrRoot}/")
    add_library(addrlib STATIC EXCLUDE_FROM_ALL ${addrSources})
    target_compile_features(addrlib PRIVATE cxx_std_17)
    if(MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_features(addrlib PRIVATE cxx_std_20)
        target_compile_options(addrlib PRIVATE -include "${CMAKE_SOURCE_DIR}/core/graphics/address/platform/mingw/BitScan.hpp")
    endif()
    target_compile_definitions(addrlib PRIVATE ADDR_FASTCALL= ADDR_ALLOW_SIMD=1 LITTLEENDIAN_CPU "DEBUG=$<IF:$<CONFIG:Debug>,1,0>")
    target_include_directories(addrlib SYSTEM PUBLIC "${addrRoot}/inc")
    target_include_directories(addrlib PRIVATE "${addrRoot}/src" "${addrRoot}/src/core" "${addrRoot}/src/chip/r800" "${addrRoot}/src/chip/gfx9" "${addrRoot}/src/chip/gfx10" "${addrRoot}/src/chip/gfx11" "${addrRoot}/src/chip/gfx12")
endfunction()
