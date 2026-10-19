cmake_minimum_required(VERSION 3.21)

# C is needed for the vendored quickjs-ng engine.
project(manga-dl VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(MANGADL_BUILD_TESTS "Build the Qt Test suite (run with ctest)" ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Svg Network Sql DBus Widgets)
find_package(libzip REQUIRED)
find_package(ZLIB REQUIRED)

qt_standard_project_setup()

# --- Vendored third-party code ---------------------------------------------------------------------
add_subdirectory(vendor/quickjs)

# --- Core library: everything that is not UI. No QML, no QGuiApplication dependency. --------------
# Linked into the app and into every test, so logic can be tested without starting the UI.
add_library(mangadl_core STATIC
    src/core/util/AppLog.h src/core/util/AppLog.cpp
    src/core/util/AppPaths.h src/core/util/AppPaths.cpp
    src/core/util/FileNaming.h src/core/util/FileNaming.cpp
    src/core/dom/Dom.h src/core/dom/Dom.cpp
    src/core/dom/HtmlParser.h src/core/dom/HtmlParser.cpp
    src/core/dom/Selector.h src/core/dom/Selector.cpp
    src/core/http/HttpService.h src/core/http/HttpService.cpp
    src/core/extensions/ExtensionModels.h src/core/extensions/ExtensionModels.cpp
    src/core/extensions/JsWorker.h src/core/extensions/JsWorker.cpp
    src/core/extensions/ExtensionManager.h src/core/extensions/ExtensionManager.cpp
    src/core/extensions/Descramble.h src/core/extensions/Descramble.cpp
    src/core/db/Entities.h
    src/core/db/MangaDatabase.h src/core/db/MangaDatabase.cpp
    src/core/db/LibraryModel.h src/core/db/LibraryModel.cpp
    src/core/db/CategoryModel.h src/core/db/CategoryModel.cpp
    src/core/db/HistoryModel.h src/core/db/HistoryModel.cpp
    src/core/db/UpdatesModel.h src/core/db/UpdatesModel.cpp
    src/core/db/DownloadsModel.h src/core/db/DownloadsModel.cpp
    src/core/downloads/CbzBuilder.h src/core/downloads/CbzBuilder.cpp
    src/core/downloads/NovelEpubBuilder.h src/core/downloads/NovelEpubBuilder.cpp
    src/core/downloads/DownloadManager.h src/core/downloads/DownloadManager.cpp
    src/core/local/ArchiveInspector.h src/core/local/ArchiveInspector.cpp
    src/core/local/EpubParser.h src/core/local/EpubParser.cpp
    src/core/local/LocalImportService.h src/core/local/LocalImportService.cpp
    src/core/settings/AppSettings.h src/core/settings/AppSettings.cpp
    src/core/sync/AuthModels.h
    src/core/sync/SupabaseAuthService.h src/core/sync/SupabaseAuthService.cpp
    src/core/sync/FirebaseAuthService.h src/core/sync/FirebaseAuthService.cpp
    src/core/sync/SupabaseSyncModels.h
    src/core/sync/SupabaseSyncService.h src/core/sync/SupabaseSyncService.cpp
    src/core/sync/SecretStorage.h src/core/sync/SecretStorage.cpp
    src/core/tracking/OAuthLoopbackListener.h src/core/tracking/OAuthLoopbackListener.cpp
    src/core/tracking/TrackerModels.h
    src/core/tracking/TrackerService.h src/core/tracking/TrackerService.cpp
    src/core/backup/MangadlBackupModels.h
    src/core/backup/TachibkModels.h
    src/core/backup/TachibkParser.h src/core/backup/TachibkParser.cpp
    src/core/backup/BackupService.h src/core/backup/BackupService.cpp
    src/core/gamification/GamificationModels.h
    src/core/gamification/GamificationService.h src/core/gamification/GamificationService.cpp
    src/core/updates/NotificationService.h src/core/updates/NotificationService.cpp
    src/core/updates/UpdateCheckerService.h src/core/updates/UpdateCheckerService.cpp
)
target_include_directories(mangadl_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)
target_link_libraries(mangadl_core PUBLIC Qt6::Core Qt6::Network Qt6::Gui Qt6::Sql Qt6::DBus quickjs libzip::zip ZLIB::ZLIB ${CMAKE_DL_LIBS})
target_compile_definitions(mangadl_core PUBLIC MANGADL_VERSION="${PROJECT_VERSION}")
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mangadl_core PRIVATE -Wall -Wextra)
endif()

# --- Application -----------------------------------------------------------------------------------
qt_add_executable(manga-dl
    src/main.cpp
    src/AppInfo.h
    src/IconProvider.h src/IconProvider.cpp
    src/ImageProvider.h src/ImageProvider.cpp
    src/MangaDetailController.h src/MangaDetailController.cpp
    src/ReaderController.h src/ReaderController.cpp
    src/SearchController.h src/SearchController.cpp
    src/BrowseSourceController.h src/BrowseSourceController.cpp
    src/ExtensionsController.h src/ExtensionsController.cpp
    src/AccountController.h src/AccountController.cpp
    src/TrackerController.h src/TrackerController.cpp
    src/BackupController.h src/BackupController.cpp
    src/SystemController.h src/SystemController.cpp
    src/StatsController.h src/StatsController.cpp
    src/ProfileController.h src/ProfileController.cpp
    src/NotificationsController.h src/NotificationsController.cpp
)

# Every .qml file under qml/ belongs to the MangaDl module.
file(GLOB_RECURSE QML_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS qml/*.qml)
file(GLOB FONT_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS assets/fonts/*.ttf)
file(GLOB EXT_JS_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS assets/extensions/*.js)

set_source_files_properties(qml/theme/Theme.qml qml/theme/Nav.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(manga-dl
    URI MangaDl
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES ${QML_FILES}
    RESOURCES ${FONT_FILES} assets/icons/manga-dl.png ${EXT_JS_FILES}
)

target_include_directories(manga-dl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(manga-dl PRIVATE
    $<$<CONFIG:Debug>:MANGADL_DEBUG_PAGES>
)

target_link_libraries(manga-dl PRIVATE
    mangadl_core
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Svg Qt6::Network Qt6::Sql Qt6::DBus Qt6::Widgets
)

# --- Tests -----------------------------------------------------------------------------------------
if(MANGADL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# --- Install ---------------------------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS manga-dl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/io.github.zenmisan.MangaDl.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES assets/icons/manga-dl.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps RENAME io.github.zenmisan.MangaDl.png)
