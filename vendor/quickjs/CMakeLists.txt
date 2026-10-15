# quickjs-ng, built as a static C library. Third-party code: warnings are silenced on purpose.
add_library(quickjs STATIC
    quickjs.c
    dtoa.c
    libregexp.c
    libunicode.c
)
set_target_properties(quickjs PROPERTIES
    LINKER_LANGUAGE C
    C_STANDARD 11
    C_EXTENSIONS ON
    POSITION_INDEPENDENT_CODE ON
)
target_include_directories(quickjs SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(quickjs PRIVATE -w -fwrapv)
find_package(Threads REQUIRED)
target_link_libraries(quickjs PUBLIC m Threads::Threads)
