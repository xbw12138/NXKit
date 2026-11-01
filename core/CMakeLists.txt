# nxkit_core: the reusable framework library (app bootstrap, config, http,
# cache, auth and the generic UI widgets).
file(GLOB_RECURSE NXKIT_CORE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

add_library(nxkit_core STATIC ${NXKIT_CORE_SOURCE})
set_property(TARGET nxkit_core PROPERTY CXX_STANDARD 17)
target_include_directories(nxkit_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(nxkit_core PRIVATE -ffunction-sections -fdata-sections)
target_link_libraries(nxkit_core PUBLIC
        borealis
        cpr::cpr
        qrcode
        lunasvg
        pystring
        z
        )
