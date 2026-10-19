#include <QDirIterator>
#include <QFontDatabase>
#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QQmlContext>
#include <QTimer>

#include "IconProvider.h"
#include "ImageProvider.h"
#include "db/MangaDatabase.h"
#include "db/LibraryModel.h"
#include "db/CategoryModel.h"
#include "db/HistoryModel.h"
#include "db/UpdatesModel.h"
#include "db/DownloadsModel.h"
#include "settings/AppSettings.h"
#include "extensions/ExtensionManager.h"
#include "util/AppLog.h"
#include "util/AppPaths.h"
#include "local/LocalImportService.h"
#include "AppInfo.h"
#include "MangaDetailController.h"
#include "ReaderController.h"
#include "SearchController.h"
#include "BrowseSourceController.h"
#include "ExtensionsController.h"
#include "AccountController.h"
#include "TrackerController.h"
#include "BackupController.h"
#include "SystemController.h"
#include "StatsController.h"
#include "ProfileController.h"
#include "NotificationsController.h"
#include "updates/UpdateCheckerService.h"
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>

using namespace mangadl;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("manga-dl"));
    QApplication::setApplicationDisplayName(QStringLiteral("manga-dl"));
    QApplication::setOrganizationDomain(QStringLiteral("zenmisan.github.io"));
    QApplication::setDesktopFileName(QStringLiteral("io.github.zenmisan.MangaDl"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/qt/qml/MangaDl/assets/icons/manga-dl.png")));

    // Profile directories and the rotating log file (XDG locations, see AppPaths).
    const bool dirsReady = AppPaths::ensureDirs();
    const bool logOpen = AppLog::init(AppPaths::stateDir());
    qInfo("manga-dl %s starting", MANGADL_VERSION);
    if (!dirsReady)
        qWarning("could not create every profile directory under %s", qPrintable(AppPaths::dataDir()));
    if (!logOpen)
        qWarning("could not open the log file in %s; logging to stderr only", qPrintable(AppPaths::stateDir()));

    // Initialize the database (SQLite, migrations, default categories)
    mangadl::MangaDatabase::instance()->initialize();

    // Initialize extension manager (builtin and user extensions)
    ExtensionManager::instance().init();

    // Bundled fonts: Anton (display), Inter (UI), PT Serif (novel reader).
    QDirIterator fonts(QStringLiteral(":/qt/qml/MangaDl/assets/fonts"), {QStringLiteral("*.ttf")});
    while (fonts.hasNext())
        QFontDatabase::addApplicationFont(fonts.next());

    // Basic is the plainest Controls style; every control the app uses is restyled in QML.
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("icon"), new IconProvider);
    engine.addImageProvider(QStringLiteral("cover"), new ImageProvider);
    engine.addImageProvider(QStringLiteral("page"), new ImageProvider);
    engine.addImageProvider(QStringLiteral("cbz"), new CbzImageProvider);

    // Register singletons in the MangaDl QML module
    qmlRegisterSingletonInstance("MangaDl", 1, 0, "AppSettings", mangadl::AppSettings::instance());
    qmlRegisterSingletonInstance("MangaDl", 1, 0, "LibraryModel", &mangadl::LibraryModel::instance());
    qmlRegisterSingletonInstance("MangaDl", 1, 0, "CategoryModel", &mangadl::CategoryModel::instance());
    qmlRegisterSingletonInstance("MangaDl", 1, 0, "HistoryModel", &mangadl::HistoryModel::instance());
    qmlRegisterSingletonInstance("MangaDl", 1, 0, "UpdatesModel", &mangadl::UpdatesModel::instance());
    qmlRegisterSingletonInstance("MangaDl", 1, 0, "DownloadsModel", &mangadl::DownloadsModel::instance());
    qmlRegisterSingletonInstance("MangaDl", 1, 0, "LocalImportService", &mangadl::LocalImportService::instance());
    auto *appInfo = new AppInfo(&engine);
    qmlRegisterSingletonInstance("MangaDl", 1, 0, "AppInfo", appInfo);

    // Register controller instantiable types in MangaDl QML module
    qmlRegisterType<MangaDetailController>("MangaDl", 1, 0, "MangaDetailController");
    qmlRegisterType<ReaderController>("MangaDl", 1, 0, "ReaderController");
    qmlRegisterType<SearchController>("MangaDl", 1, 0, "SearchController");
    qmlRegisterType<BrowseSourceController>("MangaDl", 1, 0, "BrowseSourceController");
    qmlRegisterType<ExtensionsController>("MangaDl", 1, 0, "ExtensionsController");
    qmlRegisterType<AccountController>("MangaDl", 1, 0, "AccountController");
    qmlRegisterType<TrackerController>("MangaDl", 1, 0, "TrackerController");
    qmlRegisterType<BackupController>("MangaDl", 1, 0, "BackupController");
    qmlRegisterType<SystemController>("MangaDl", 1, 0, "SystemController");
    qmlRegisterType<StatsController>("MangaDl", 1, 0, "StatsController");
    qmlRegisterType<ProfileController>("MangaDl", 1, 0, "ProfileController");
    qmlRegisterType<NotificationsController>("MangaDl", 1, 0, "NotificationsController");

    auto *accountController = new AccountController(&engine);
    auto *trackerController = new TrackerController(&engine);
    auto *backupController = new BackupController(&engine);
    auto *systemController = new SystemController(&engine);
    auto *statsController = new StatsController(mangadl::MangaDatabase::instance(), &engine);
    auto *profileController = new ProfileController(mangadl::MangaDatabase::instance(), &engine);
    auto *notificationsController = new NotificationsController(mangadl::MangaDatabase::instance(), &engine);

    qmlRegisterSingletonInstance("MangaDl", 1, 0, "AccountController", accountController);
    qmlRegisterSingletonInstance("MangaDl", 1, 0, "TrackerController", trackerController);
    qmlRegisterSingletonInstance("MangaDl", 1, 0, "BackupController", backupController);
    qmlRegisterSingletonInstance("MangaDl", 1, 0, "SystemController", systemController);
    qmlRegisterSingletonInstance("MangaDl", 1, 0, "StatsController", statsController);
    qmlRegisterSingletonInstance("MangaDl", 1, 0, "ProfileController", profileController);
    qmlRegisterSingletonInstance("MangaDl", 1, 0, "NotificationsController", notificationsController);
    qmlRegisterSingletonInstance("MangaDl", 1, 0, "UpdateCheckerService", &mangadl::UpdateCheckerService::instance());

    // Also expose to root context for convenient property access
    auto *ctx = engine.rootContext();
    ctx->setContextProperty(QStringLiteral("AppInfo"), appInfo);
    ctx->setContextProperty(QStringLiteral("AppSettings"), mangadl::AppSettings::instance());
    ctx->setContextProperty(QStringLiteral("appSettings"), mangadl::AppSettings::instance());
    ctx->setContextProperty(QStringLiteral("LibraryModel"), &mangadl::LibraryModel::instance());
    ctx->setContextProperty(QStringLiteral("libraryModel"), &mangadl::LibraryModel::instance());
    ctx->setContextProperty(QStringLiteral("CategoryModel"), &mangadl::CategoryModel::instance());
    ctx->setContextProperty(QStringLiteral("categoryModel"), &mangadl::CategoryModel::instance());
    ctx->setContextProperty(QStringLiteral("HistoryModel"), &mangadl::HistoryModel::instance());
    ctx->setContextProperty(QStringLiteral("historyModel"), &mangadl::HistoryModel::instance());
    ctx->setContextProperty(QStringLiteral("UpdatesModel"), &mangadl::UpdatesModel::instance());
    ctx->setContextProperty(QStringLiteral("updatesModel"), &mangadl::UpdatesModel::instance());
    ctx->setContextProperty(QStringLiteral("DownloadsModel"), &mangadl::DownloadsModel::instance());
    ctx->setContextProperty(QStringLiteral("downloadsModel"), &mangadl::DownloadsModel::instance());
    ctx->setContextProperty(QStringLiteral("LocalImportService"), &mangadl::LocalImportService::instance());
    ctx->setContextProperty(QStringLiteral("localImportService"), &mangadl::LocalImportService::instance());
    ctx->setContextProperty(QStringLiteral("AccountController"), accountController);
    ctx->setContextProperty(QStringLiteral("accountController"), accountController);
    ctx->setContextProperty(QStringLiteral("TrackerController"), trackerController);
    ctx->setContextProperty(QStringLiteral("trackerController"), trackerController);
    ctx->setContextProperty(QStringLiteral("BackupController"), backupController);
    ctx->setContextProperty(QStringLiteral("backupController"), backupController);
    ctx->setContextProperty(QStringLiteral("SystemController"), systemController);
    ctx->setContextProperty(QStringLiteral("systemController"), systemController);
    ctx->setContextProperty(QStringLiteral("StatsController"), statsController);
    ctx->setContextProperty(QStringLiteral("statsController"), statsController);
    ctx->setContextProperty(QStringLiteral("ProfileController"), profileController);
    ctx->setContextProperty(QStringLiteral("profileController"), profileController);
    ctx->setContextProperty(QStringLiteral("NotificationsController"), notificationsController);
    ctx->setContextProperty(QStringLiteral("notificationsController"), notificationsController);
    ctx->setContextProperty(QStringLiteral("UpdateCheckerService"), &mangadl::UpdateCheckerService::instance());
    ctx->setContextProperty(QStringLiteral("updateCheckerService"), &mangadl::UpdateCheckerService::instance());

    // System Tray integration
    QSystemTrayIcon *trayIcon = nullptr;
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayIcon = new QSystemTrayIcon(QIcon(QStringLiteral(":/qt/qml/MangaDl/assets/icons/manga-dl.png")), &app);
        trayIcon->setToolTip(QStringLiteral("manga-dl"));

        auto *trayMenu = new QMenu();
        trayMenu->setStyleSheet(QStringLiteral(
            "QMenu {"
            "  background-color: #18181b;"
            "  color: #f4f4f5;"
            "  border: 1px solid #3f3f46;"
            "  border-radius: 8px;"
            "  padding: 6px;"
            "  font-size: 13px;"
            "}"
            "QMenu::item {"
            "  padding: 6px 24px 6px 12px;"
            "  border-radius: 4px;"
            "  background-color: transparent;"
            "}"
            "QMenu::item:selected {"
            "  background-color: #e11d48;"
            "  color: #ffffff;"
            "}"
            "QMenu::separator {"
            "  height: 1px;"
            "  background-color: #27272a;"
            "  margin: 4px 6px;"
            "}"
        ));
        auto *showAction = trayMenu->addAction(QStringLiteral("Show manga-dl"));
        auto *hideAction = trayMenu->addAction(QStringLiteral("Hide"));
        trayMenu->addSeparator();
        auto *quitAction = trayMenu->addAction(QStringLiteral("Quit"));

        QObject::connect(showAction, &QAction::triggered, [&engine]() {
            if (!engine.rootObjects().isEmpty()) {
                if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first())) {
                    window->show();
                    window->raise();
                    window->requestActivate();
                }
            }
        });

        QObject::connect(hideAction, &QAction::triggered, [&engine]() {
            if (!engine.rootObjects().isEmpty()) {
                if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first())) {
                    window->hide();
                }
            }
        });

        QObject::connect(quitAction, &QAction::triggered, &app, &QCoreApplication::quit);

        trayIcon->setContextMenu(trayMenu);

        QObject::connect(trayIcon, &QSystemTrayIcon::activated, [&engine](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                if (!engine.rootObjects().isEmpty()) {
                    if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first())) {
                        if (window->isVisible()) {
                            window->hide();
                        } else {
                            window->show();
                            window->raise();
                            window->requestActivate();
                        }
                    }
                }
            }
        });
        trayIcon->show();
    }

    // Initialize autostart if configured
    if (mangadl::AppSettings::instance()->launchOnStartup()) {
        mangadl::SystemController::setAutostart(true);
    }

    // Start auto update-check timer
    if (mangadl::AppSettings::instance()->autoCheckUpdates()) {
        mangadl::UpdateCheckerService::instance().startAutoTimer(30);
    }

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    // (QQmlApplicationEngine::loadFromModule needs Qt 6.5; a URL works on 6.4+.)
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/MangaDl/qml/Main.qml")));

    // Dev helper: MANGADL_SCREENSHOT=out.png saves the window after it settles, then quits.
    // Combine with `--page <Name>` to capture any screen (used to check against the designs).
    if (const QString shot = qEnvironmentVariable("MANGADL_SCREENSHOT"); !shot.isEmpty() && !engine.rootObjects().isEmpty()) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
        const int delay = qEnvironmentVariableIsSet("MANGADL_SCREENSHOT_DELAY") ? qEnvironmentVariableIntValue("MANGADL_SCREENSHOT_DELAY") : 1200;
        QTimer::singleShot(delay, window, [window, shot] {
            window->grabWindow().save(shot);
            QCoreApplication::quit();
        });
    }

    const int exitCode = app.exec();
    qInfo("manga-dl exiting (code %d)", exitCode);
    AppLog::shutdown();
    return exitCode;
}
