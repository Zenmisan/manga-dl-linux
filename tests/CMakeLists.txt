find_package(Qt6 REQUIRED COMPONENTS Test)

# mangadl_add_test(<n> [extra libs...])  - builds <n>.cpp and registers it with ctest.
function(mangadl_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE Qt6::Test mangadl_core ${ARGN})
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

mangadl_add_test(tst_smoke)
mangadl_add_test(tst_apppaths)
mangadl_add_test(tst_applog)
mangadl_add_test(tst_quickjs quickjs)
mangadl_add_test(tst_selector)
mangadl_add_test(tst_selector_extra)
mangadl_add_test(tst_extension_selectors)
mangadl_add_test(tst_jsworker)
mangadl_add_test(tst_descramble)
mangadl_add_test(tst_extensions)
mangadl_add_test(tst_r3)
mangadl_add_test(tst_database)
mangadl_add_test(tst_settings)
mangadl_add_test(tst_models)

add_executable(tst_imageprovider tst_imageprovider.cpp ${CMAKE_SOURCE_DIR}/src/ImageProvider.cpp)
target_include_directories(tst_imageprovider PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_imageprovider PRIVATE Qt6::Test Qt6::Quick mangadl_core)
add_test(NAME tst_imageprovider COMMAND tst_imageprovider)
set_tests_properties(tst_imageprovider PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(tst_controllers tst_controllers.cpp ${CMAKE_SOURCE_DIR}/src/MangaDetailController.cpp ${CMAKE_SOURCE_DIR}/src/ReaderController.cpp)
target_include_directories(tst_controllers PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_controllers PRIVATE Qt6::Test Qt6::Quick Qt6::Qml mangadl_core)
add_test(NAME tst_controllers COMMAND tst_controllers)
set_tests_properties(tst_controllers PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(tst_discovery tst_discovery.cpp ${CMAKE_SOURCE_DIR}/src/SearchController.cpp ${CMAKE_SOURCE_DIR}/src/BrowseSourceController.cpp ${CMAKE_SOURCE_DIR}/src/ExtensionsController.cpp)
target_include_directories(tst_discovery PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_discovery PRIVATE Qt6::Test Qt6::Quick Qt6::Qml mangadl_core)
add_test(NAME tst_discovery COMMAND tst_discovery)
set_tests_properties(tst_discovery PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(tst_downloads tst_downloads.cpp)
target_include_directories(tst_downloads PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_downloads PRIVATE Qt6::Test Qt6::Quick Qt6::Qml mangadl_core)
add_test(NAME tst_downloads COMMAND tst_downloads)
set_tests_properties(tst_downloads PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(tst_sync tst_sync.cpp)
target_include_directories(tst_sync PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_sync PRIVATE Qt6::Test Qt6::Network ZLIB::ZLIB mangadl_core)
add_test(NAME tst_sync COMMAND tst_sync)
set_tests_properties(tst_sync PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(tst_services tst_services.cpp ${CMAKE_SOURCE_DIR}/src/SystemController.cpp)
target_include_directories(tst_services PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_services PRIVATE Qt6::Test Qt6::Network Qt6::Sql mangadl_core)
add_test(NAME tst_services COMMAND tst_services)
set_tests_properties(tst_services PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Shared by the R3 replay test and the live capture tool.
add_executable(r3-capture
    ${CMAKE_SOURCE_DIR}/tools/r3-capture.cpp
    R3Support.cpp
)
target_link_libraries(r3-capture PRIVATE Qt6::Core Qt6::Network mangadl_core)
target_include_directories(r3-capture PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src/core)

target_sources(tst_r3 PRIVATE R3Support.cpp)
target_include_directories(tst_r3 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# These tests load the real extension scripts (copied there by tools/sync-extensions.sh).
set(MANGADL_ASSETS "${CMAKE_SOURCE_DIR}/assets")
set(MANGADL_R3_FIXTURES "${CMAKE_SOURCE_DIR}/tests/fixtures/r3")
foreach(t tst_jsworker tst_extensions tst_extension_selectors tst_r3 r3-capture tst_discovery)
    target_compile_definitions(${t} PRIVATE
        MANGADL_ASSETS_DIR="${MANGADL_ASSETS}"
        MANGADL_FIXTURES_DIR="${MANGADL_R3_FIXTURES}"
    )
endforeach()
