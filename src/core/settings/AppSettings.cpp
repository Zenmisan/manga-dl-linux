#include "AppSettings.h"
#include "util/AppPaths.h"

#include <QDir>
#include <QFileInfo>

namespace mangadl {

static AppSettings *s_settingsInstance = nullptr;

static const char *kDefaultBackendUrl = "https://manga-dl.onrender.com";
static const char *kDefaultSupabaseUrl = "https://gyivwfweldwvzccbpgoz.supabase.co";
static const char *kDefaultSupabaseAnonKey =
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
    "eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imd5aXZ3ZndlbGR3dnpjY2JwZ296Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODAxNjg2NTUsImV4cCI6MjA5NTc0NDY1NX0."
    "XcEJk1fyv-QxSehPUIeRR77ocIkPIZzDyc4DDfrr6XQ";

static const char *kDefaultAnilistClientId = "53109";
static const char *kDefaultAnilistClientSecret = "7xKnM4QU77qtWWLXOw3V0rx1mvLnmriX9ZcI3Cki";
static const char *kDefaultAnilistRedirectUri = "http://localhost:5678/mal-callback";

static const char *kDefaultMalClientId = "4ee644500f513dd7887120b026b65f39";
static const char *kDefaultMalRedirectUri = "http://localhost:5678/mal-callback";
static const char *kDefaultGoogleRedirectUri = "http://localhost:5678/google-callback";

AppSettings *AppSettings::instance()
{
    if (!s_settingsInstance) {
        s_settingsInstance = new AppSettings();
    }
    return s_settingsInstance;
}

void AppSettings::setInstance(AppSettings *customInstance)
{
    s_settingsInstance = customInstance;
}

AppSettings::AppSettings(const QString &configFilePath, QObject *parent)
    : QObject(parent)
{
    QString path = configFilePath;
    if (path.isEmpty()) {
        path = AppPaths::configDir() + QStringLiteral("/settings.ini");
    }
    QFileInfo fi(path);
    QDir().mkpath(fi.absolutePath());
    m_settings = std::make_unique<QSettings>(path, QSettings::IniFormat);
}

AppSettings::~AppSettings()
{
    if (s_settingsInstance == this) {
        s_settingsInstance = nullptr;
    }
}

void AppSettings::resetToDefaults()
{
    m_settings->clear();
    m_settings->sync();
    emit settingsChanged();
    emit themeChanged();
    emit accentColorChanged();
    emit ambilightChanged();
    emit backendUrlChanged();
    emit apiKeyChanged();
    emit supabaseUrlChanged();
    emit supabaseAnonKeyChanged();
    emit userEmailChanged();
    emit userIdChanged();
    emit displayNameChanged();
    emit bioChanged();
    emit avatarUrlChanged();
    emit showReadingActivityChanged();
    emit launchOnStartupChanged();
    emit keepRunningInTrayChanged();
    emit desktopNotificationsChanged();
    emit discordRpcChanged();
    emit incognitoChanged();
    emit autoDownloadNewChanged();
    emit readerModeChanged();
    emit readingDirectionChanged();
    emit twoPageSpreadChanged();
    emit cropBordersChanged();
    emit webtoonSidePaddingChanged();
    emit clickZonesChanged();
    emit mouseWheelActionChanged();
    emit pageFitChanged();
    emit readerBackgroundChanged();
    emit showPageNumberChanged();
    emit openFullscreenChanged();
    emit imageBrightnessChanged();
    emit imageGrayscaleChanged();
    emit imageInvertChanged();
    emit sharpenImagesChanged();
    emit novelFontSizeChanged();
    emit novelFontFamilyChanged();
    emit novelLineHeightChanged();
    emit novelThemeChanged();
    emit gridColumnsChanged();
    emit unreadBadgesChanged();
    emit downloadedBadgesChanged();
    emit continueReadingRowChanged();
    emit checkNewChaptersChanged();
    emit onlyUnmeteredChanged();
    emit skipCompletedSeriesChanged();
    emit downloadPathChanged();
    emit parallelDownloadsChanged();
    emit downloadFormatChanged();
    emit syncLibraryChanged();
    emit syncProgressChanged();
    emit syncSettingsChanged();
    emit lastSyncTimeChanged();
    emit autoBackupScheduleChanged();
    emit lastBackupDateChanged();
    emit autoCheckUpdatesChanged();
    emit lastUpdateCheckChanged();
    emit installUpdatesAutomaticallyChanged();
    emit anilistClientIdChanged();
    emit anilistClientSecretChanged();
    emit anilistRedirectUriChanged();
    emit anilistUsernameChanged();
    emit anilistConnectedChanged();
    emit malClientIdChanged();
    emit malRedirectUriChanged();
    emit malUsernameChanged();
    emit malConnectedChanged();
    emit googleRedirectUriChanged();
    emit autoSyncTrackersChanged();
    emit markTrackerReadingOnFirstChapterChanged();
    emit markTrackerCompletedOnFinishChanged();
    emit pullProgressFromTrackersChanged();
    emit libraryViewModeChanged();
    emit libraryDensityChanged();
    emit libraryContentTypeChanged();
    emit libraryFilterModeChanged();
    emit librarySortModeChanged();
    emit kitsuTokenChanged();
    emit mangaUpdatesTokenChanged();
    emit shikimoriTokenChanged();
    emit bangumiTokenChanged();
    emit pinnedBadgesChanged();
    emit monthlyChapterGoalChanged();
    emit yearlyMangaGoalChanged();
    emit enabledSearchSourcesChanged();
    emit disabledExtensionsChanged();
    emit customExtensionReposChanged();
    emit appLockEnabledChanged();
    emit appLockPinChanged();
    emit komgaEnabledChanged();
    emit komgaUrlChanged();
    emit komgaUsernameChanged();
    emit komgaPasswordChanged();
    emit suwayomiEnabledChanged();
    emit suwayomiUrlChanged();
    emit suwayomiApiKeyChanged();
    emit onboardingCompletedChanged();
    emit libraryTourCompletedChanged();
    emit settingsTourCompletedChanged();
}

void AppSettings::sync()
{
    m_settings->sync();
}

// -----------------------------------------------------------------------------
// Appearance
// -----------------------------------------------------------------------------

QString AppSettings::theme() const { return m_settings->value(QStringLiteral("appearance/theme"), QStringLiteral("Dark")).toString(); }
void AppSettings::setTheme(const QString &val) {
    if (theme() != val) { m_settings->setValue(QStringLiteral("appearance/theme"), val); emit themeChanged(); emit settingsChanged(); }
}

QString AppSettings::accentColor() const { return m_settings->value(QStringLiteral("appearance/accentColor"), QStringLiteral("Red")).toString(); }
void AppSettings::setAccentColor(const QString &val) {
    if (accentColor() != val) { m_settings->setValue(QStringLiteral("appearance/accentColor"), val); emit accentColorChanged(); emit settingsChanged(); }
}

bool AppSettings::ambilight() const { return m_settings->value(QStringLiteral("appearance/ambilight"), true).toBool(); }
void AppSettings::setAmbilight(bool val) {
    if (ambilight() != val) { m_settings->setValue(QStringLiteral("appearance/ambilight"), val); emit ambilightChanged(); emit settingsChanged(); }
}

// -----------------------------------------------------------------------------
// Connection
// -----------------------------------------------------------------------------

QString AppSettings::backendUrl() const { return m_settings->value(QStringLiteral("connection/backendUrl"), QString::fromUtf8(kDefaultBackendUrl)).toString(); }
void AppSettings::setBackendUrl(const QString &val) {
    if (backendUrl() != val) { m_settings->setValue(QStringLiteral("connection/backendUrl"), val); emit backendUrlChanged(); emit settingsChanged(); }
}

QString AppSettings::apiKey() const { return m_settings->value(QStringLiteral("connection/apiKey"), QString()).toString(); }
void AppSettings::setApiKey(const QString &val) {
    if (apiKey() != val) { m_settings->setValue(QStringLiteral("connection/apiKey"), val); emit apiKeyChanged(); emit settingsChanged(); }
}

QString AppSettings::supabaseUrl() const {
    const QString envUrl = QString::fromLocal8Bit(qgetenv("SUPABASE_URL"));
    if (!envUrl.trimmed().isEmpty()) return envUrl.trimmed();
    const QString nextEnv = QString::fromLocal8Bit(qgetenv("NEXT_PUBLIC_SUPABASE_URL"));
    if (!nextEnv.trimmed().isEmpty()) return nextEnv.trimmed();
    return m_settings->value(QStringLiteral("connection/supabaseUrl"), QString::fromUtf8(kDefaultSupabaseUrl)).toString();
}
void AppSettings::setSupabaseUrl(const QString &val) {
    if (supabaseUrl() != val) { m_settings->setValue(QStringLiteral("connection/supabaseUrl"), val); emit supabaseUrlChanged(); emit settingsChanged(); }
}

QString AppSettings::supabaseAnonKey() const {
    const QString envKey = QString::fromLocal8Bit(qgetenv("SUPABASE_ANON_KEY"));
    if (!envKey.trimmed().isEmpty()) return envKey.trimmed();
    const QString nextKey = QString::fromLocal8Bit(qgetenv("NEXT_PUBLIC_SUPABASE_ANON_KEY"));
    if (!nextKey.trimmed().isEmpty()) return nextKey.trimmed();
    return m_settings->value(QStringLiteral("connection/supabaseAnonKey"), QString::fromUtf8(kDefaultSupabaseAnonKey)).toString();
}
void AppSettings::setSupabaseAnonKey(const QString &val) {
    if (supabaseAnonKey() != val) { m_settings->setValue(QStringLiteral("connection/supabaseAnonKey"), val); emit supabaseAnonKeyChanged(); emit settingsChanged(); }
}

bool AppSettings::hasSupabase() const {
    return !supabaseUrl().trimmed().isEmpty() && !supabaseAnonKey().trimmed().isEmpty();
}

// -----------------------------------------------------------------------------
// Account
// -----------------------------------------------------------------------------

QString AppSettings::userEmail() const { return m_settings->value(QStringLiteral("account/userEmail"), QString()).toString(); }
void AppSettings::setUserEmail(const QString &val) {
    if (userEmail() != val) { m_settings->setValue(QStringLiteral("account/userEmail"), val); emit userEmailChanged(); emit settingsChanged(); }
}

QString AppSettings::userId() const { return m_settings->value(QStringLiteral("account/userId"), QString()).toString(); }
void AppSettings::setUserId(const QString &val) {
    if (userId() != val) { m_settings->setValue(QStringLiteral("account/userId"), val); emit userIdChanged(); emit settingsChanged(); }
}

QString AppSettings::displayName() const { return m_settings->value(QStringLiteral("account/displayName"), QString()).toString(); }
void AppSettings::setDisplayName(const QString &val) {
    if (displayName() != val) { m_settings->setValue(QStringLiteral("account/displayName"), val); emit displayNameChanged(); emit settingsChanged(); }
}

QString AppSettings::bio() const { return m_settings->value(QStringLiteral("account/bio"), QString()).toString(); }
void AppSettings::setBio(const QString &val) {
    if (bio() != val) { m_settings->setValue(QStringLiteral("account/bio"), val); emit bioChanged(); emit settingsChanged(); }
}

QString AppSettings::avatarUrl() const { return m_settings->value(QStringLiteral("account/avatarUrl"), QString()).toString(); }
void AppSettings::setAvatarUrl(const QString &val) {
    if (avatarUrl() != val) { m_settings->setValue(QStringLiteral("account/avatarUrl"), val); emit avatarUrlChanged(); emit settingsChanged(); }
}

bool AppSettings::showReadingActivity() const { return m_settings->value(QStringLiteral("account/showReadingActivity"), true).toBool(); }
void AppSettings::setShowReadingActivity(bool val) {
    if (showReadingActivity() != val) { m_settings->setValue(QStringLiteral("account/showReadingActivity"), val); emit showReadingActivityChanged(); emit settingsChanged(); }
}

// -----------------------------------------------------------------------------
// Desktop
// -----------------------------------------------------------------------------

bool AppSettings::launchOnStartup() const { return m_settings->value(QStringLiteral("desktop/launchOnStartup"), false).toBool(); }
void AppSettings::setLaunchOnStartup(bool val) {
    if (launchOnStartup() != val) { m_settings->setValue(QStringLiteral("desktop/launchOnStartup"), val); emit launchOnStartupChanged(); emit settingsChanged(); }
}

bool AppSettings::keepRunningInTray() const { return m_settings->value(QStringLiteral("desktop/keepRunningInTray"), true).toBool(); }
void AppSettings::setKeepRunningInTray(bool val) {
    if (keepRunningInTray() != val) { m_settings->setValue(QStringLiteral("desktop/keepRunningInTray"), val); emit keepRunningInTrayChanged(); emit settingsChanged(); }
}

bool AppSettings::desktopNotifications() const { return m_settings->value(QStringLiteral("desktop/desktopNotifications"), true).toBool(); }
void AppSettings::setDesktopNotifications(bool val) {
    if (desktopNotifications() != val) { m_settings->setValue(QStringLiteral("desktop/desktopNotifications"), val); emit desktopNotificationsChanged(); emit settingsChanged(); }
}

bool AppSettings::discordRpc() const { return m_settings->value(QStringLiteral("desktop/discordRpc"), false).toBool(); }
void AppSettings::setDiscordRpc(bool val) {
    if (discordRpc() != val) { m_settings->setValue(QStringLiteral("desktop/discordRpc"), val); emit discordRpcChanged(); emit settingsChanged(); }
}

// -----------------------------------------------------------------------------
// Behaviour
// -----------------------------------------------------------------------------

bool AppSettings::incognito() const { return m_settings->value(QStringLiteral("behaviour/incognito"), false).toBool(); }
void AppSettings::setIncognito(bool val) {
    if (incognito() != val) { m_settings->setValue(QStringLiteral("behaviour/incognito"), val); emit incognitoChanged(); emit settingsChanged(); }
}

bool AppSettings::autoDownloadNew() const { return m_settings->value(QStringLiteral("behaviour/autoDownloadNew"), false).toBool(); }
void AppSettings::setAutoDownloadNew(bool val) {
    if (autoDownloadNew() != val) { m_settings->setValue(QStringLiteral("behaviour/autoDownloadNew"), val); emit autoDownloadNewChanged(); emit settingsChanged(); }
}

// -----------------------------------------------------------------------------
// Reader
// -----------------------------------------------------------------------------

QString AppSettings::readerMode() const { return m_settings->value(QStringLiteral("reader/readerMode"), QStringLiteral("Right to left")).toString(); }
void AppSettings::setReaderMode(const QString &val) {
    if (readerMode() != val) { m_settings->setValue(QStringLiteral("reader/readerMode"), val); emit readerModeChanged(); emit settingsChanged(); }
}

QString AppSettings::readingDirection() const { return m_settings->value(QStringLiteral("reader/readingDirection"), QStringLiteral("RTL")).toString(); }
void AppSettings::setReadingDirection(const QString &val) {
    if (readingDirection() != val) { m_settings->setValue(QStringLiteral("reader/readingDirection"), val); emit readingDirectionChanged(); emit settingsChanged(); }
}

bool AppSettings::twoPageSpread() const { return m_settings->value(QStringLiteral("reader/twoPageSpread"), true).toBool(); }
void AppSettings::setTwoPageSpread(bool val) {
    if (twoPageSpread() != val) { m_settings->setValue(QStringLiteral("reader/twoPageSpread"), val); emit twoPageSpreadChanged(); emit settingsChanged(); }
}

bool AppSettings::cropBorders() const { return m_settings->value(QStringLiteral("reader/cropBorders"), false).toBool(); }
void AppSettings::setCropBorders(bool val) {
    if (cropBorders() != val) { m_settings->setValue(QStringLiteral("reader/cropBorders"), val); emit cropBordersChanged(); emit settingsChanged(); }
}

int AppSettings::webtoonSidePadding() const { return m_settings->value(QStringLiteral("reader/webtoonSidePadding"), 15).toInt(); }
void AppSettings::setWebtoonSidePadding(int val) {
    if (webtoonSidePadding() != val) { m_settings->setValue(QStringLiteral("reader/webtoonSidePadding"), val); emit webtoonSidePaddingChanged(); emit settingsChanged(); }
}

bool AppSettings::clickZones() const { return m_settings->value(QStringLiteral("reader/clickZones"), true).toBool(); }
void AppSettings::setClickZones(bool val) {
    if (clickZones() != val) { m_settings->setValue(QStringLiteral("reader/clickZones"), val); emit clickZonesChanged(); emit settingsChanged(); }
}

QString AppSettings::mouseWheelAction() const { return m_settings->value(QStringLiteral("reader/mouseWheelAction"), QStringLiteral("Scroll")).toString(); }
void AppSettings::setMouseWheelAction(const QString &val) {
    if (mouseWheelAction() != val) { m_settings->setValue(QStringLiteral("reader/mouseWheelAction"), val); emit mouseWheelActionChanged(); emit settingsChanged(); }
}

QString AppSettings::pageFit() const { return m_settings->value(QStringLiteral("reader/pageFit"), QStringLiteral("Fit height")).toString(); }
void AppSettings::setPageFit(const QString &val) {
    if (pageFit() != val) { m_settings->setValue(QStringLiteral("reader/pageFit"), val); emit pageFitChanged(); emit settingsChanged(); }
}

QString AppSettings::readerBackground() const { return m_settings->value(QStringLiteral("reader/readerBackground"), QStringLiteral("Black")).toString(); }
void AppSettings::setReaderBackground(const QString &val) {
    if (readerBackground() != val) { m_settings->setValue(QStringLiteral("reader/readerBackground"), val); emit readerBackgroundChanged(); emit settingsChanged(); }
}

bool AppSettings::showPageNumber() const { return m_settings->value(QStringLiteral("reader/showPageNumber"), true).toBool(); }
void AppSettings::setShowPageNumber(bool val) {
    if (showPageNumber() != val) { m_settings->setValue(QStringLiteral("reader/showPageNumber"), val); emit showPageNumberChanged(); emit settingsChanged(); }
}

bool AppSettings::openFullscreen() const { return m_settings->value(QStringLiteral("reader/openFullscreen"), false).toBool(); }
void AppSettings::setOpenFullscreen(bool val) {
    if (openFullscreen() != val) { m_settings->setValue(QStringLiteral("reader/openFullscreen"), val); emit openFullscreenChanged(); emit settingsChanged(); }
}

int AppSettings::imageBrightness() const { return m_settings->value(QStringLiteral("reader/imageBrightness"), 0).toInt(); }
void AppSettings::setImageBrightness(int val) {
    if (imageBrightness() != val) { m_settings->setValue(QStringLiteral("reader/imageBrightness"), val); emit imageBrightnessChanged(); emit settingsChanged(); }
}

bool AppSettings::imageGrayscale() const { return m_settings->value(QStringLiteral("reader/imageGrayscale"), false).toBool(); }
void AppSettings::setImageGrayscale(bool val) {
    if (imageGrayscale() != val) { m_settings->setValue(QStringLiteral("reader/imageGrayscale"), val); emit imageGrayscaleChanged(); emit settingsChanged(); }
}

bool AppSettings::imageInvert() const { return m_settings->value(QStringLiteral("reader/imageInvert"), false).toBool(); }
void AppSettings::setImageInvert(bool val) {
    if (imageInvert() != val) { m_settings->setValue(QStringLiteral("reader/imageInvert"), val); emit imageInvertChanged(); emit settingsChanged(); }
}

bool AppSettings::sharpenImages() const { return m_settings->value(QStringLiteral("reader/sharpenImages"), true).toBool(); }
void AppSettings::setSharpenImages(bool val) {
    if (sharpenImages() != val) { m_settings->setValue(QStringLiteral("reader/sharpenImages"), val); emit sharpenImagesChanged(); emit settingsChanged(); }
}

// -----------------------------------------------------------------------------
// Novel Reader
// -----------------------------------------------------------------------------

int AppSettings::novelFontSize() const { return m_settings->value(QStringLiteral("novelReader/novelFontSize"), 18).toInt(); }
void AppSettings::setNovelFontSize(int val) {
    if (novelFontSize() != val) { m_settings->setValue(QStringLiteral("novelReader/novelFontSize"), val); emit novelFontSizeChanged(); emit settingsChanged(); }
}

QString AppSettings::novelFontFamily() const { return m_settings->value(QStringLiteral("novelReader/novelFontFamily"), QStringLiteral("Inter")).toString(); }
void AppSettings::setNovelFontFamily(const QString &val) {
    if (novelFontFamily() != val) { m_settings->setValue(QStringLiteral("novelReader/novelFontFamily"), val); emit novelFontFamilyChanged(); emit settingsChanged(); }
}

double AppSettings::novelLineHeight() const { return m_settings->value(QStringLiteral("novelReader/novelLineHeight"), 1.6).toDouble(); }
void AppSettings::setNovelLineHeight(double val) {
    if (qAbs(novelLineHeight() - val) > 0.001) { m_settings->setValue(QStringLiteral("novelReader/novelLineHeight"), val); emit novelLineHeightChanged(); emit settingsChanged(); }
}

QString AppSettings::novelTheme() const { return m_settings->value(QStringLiteral("novelReader/novelTheme"), QStringLiteral("Dark")).toString(); }
void AppSettings::setNovelTheme(const QString &val) {
    if (novelTheme() != val) { m_settings->setValue(QStringLiteral("novelReader/novelTheme"), val); emit novelThemeChanged(); emit settingsChanged(); }
}

// -----------------------------------------------------------------------------
// Library
// -----------------------------------------------------------------------------

QString AppSettings::gridColumns() const { return m_settings->value(QStringLiteral("library/gridColumns"), QStringLiteral("Auto")).toString(); }
void AppSettings::setGridColumns(const QString &val) {
    if (gridColumns() != val) { m_settings->setValue(QStringLiteral("library/gridColumns"), val); emit gridColumnsChanged(); emit settingsChanged(); }
}

QString AppSettings::libraryViewMode() const { return m_settings->value(QStringLiteral("library/viewMode"), QStringLiteral("grid")).toString(); }
void AppSettings::setLibraryViewMode(const QString &val) {
    if (libraryViewMode() != val) { m_settings->setValue(QStringLiteral("library/viewMode"), val); emit libraryViewModeChanged(); emit settingsChanged(); }
}

QString AppSettings::libraryDensity() const { return m_settings->value(QStringLiteral("library/density"), QStringLiteral("normal")).toString(); }
void AppSettings::setLibraryDensity(const QString &val) {
    if (libraryDensity() != val) { m_settings->setValue(QStringLiteral("library/density"), val); emit libraryDensityChanged(); emit settingsChanged(); }
}

QString AppSettings::libraryContentType() const { return m_settings->value(QStringLiteral("library/contentType"), QStringLiteral("all")).toString(); }
void AppSettings::setLibraryContentType(const QString &val) {
    if (libraryContentType() != val) { m_settings->setValue(QStringLiteral("library/contentType"), val); emit libraryContentTypeChanged(); emit settingsChanged(); }
}

QString AppSettings::libraryFilterMode() const { return m_settings->value(QStringLiteral("library/filterMode"), QStringLiteral("all")).toString(); }
void AppSettings::setLibraryFilterMode(const QString &val) {
    if (libraryFilterMode() != val) { m_settings->setValue(QStringLiteral("library/filterMode"), val); emit libraryFilterModeChanged(); emit settingsChanged(); }
}

QString AppSettings::librarySortMode() const { return m_settings->value(QStringLiteral("library/sortMode"), QStringLiteral("lastRead")).toString(); }
void AppSettings::setLibrarySortMode(const QString &val) {
    if (librarySortMode() != val) { m_settings->setValue(QStringLiteral("library/sortMode"), val); emit librarySortModeChanged(); emit settingsChanged(); }
}

bool AppSettings::unreadBadges() const { return m_settings->value(QStringLiteral("library/unreadBadges"), true).toBool(); }
void AppSettings::setUnreadBadges(bool val) {
    if (unreadBadges() != val) { m_settings->setValue(QStringLiteral("library/unreadBadges"), val); emit unreadBadgesChanged(); emit settingsChanged(); }
}

bool AppSettings::downloadedBadges() const { return m_settings->value(QStringLiteral("library/downloadedBadges"), true).toBool(); }
void AppSettings::setDownloadedBadges(bool val) {
    if (downloadedBadges() != val) { m_settings->setValue(QStringLiteral("library/downloadedBadges"), val); emit downloadedBadgesChanged(); emit settingsChanged(); }
}

bool AppSettings::continueReadingRow() const { return m_settings->value(QStringLiteral("library/continueReadingRow"), true).toBool(); }
void AppSettings::setContinueReadingRow(bool val) {
    if (continueReadingRow() != val) { m_settings->setValue(QStringLiteral("library/continueReadingRow"), val); emit continueReadingRowChanged(); emit settingsChanged(); }
}

QString AppSettings::checkNewChapters() const { return m_settings->value(QStringLiteral("library/checkNewChapters"), QStringLiteral("Every 12 hours")).toString(); }
void AppSettings::setCheckNewChapters(const QString &val) {
    if (checkNewChapters() != val) { m_settings->setValue(QStringLiteral("library/checkNewChapters"), val); emit checkNewChaptersChanged(); emit settingsChanged(); }
}

bool AppSettings::onlyUnmetered() const { return m_settings->value(QStringLiteral("library/onlyUnmetered"), true).toBool(); }
void AppSettings::setOnlyUnmetered(bool val) {
    if (onlyUnmetered() != val) { m_settings->setValue(QStringLiteral("library/onlyUnmetered"), val); emit onlyUnmeteredChanged(); emit settingsChanged(); }
}

bool AppSettings::skipCompletedSeries() const { return m_settings->value(QStringLiteral("library/skipCompletedSeries"), true).toBool(); }
void AppSettings::setSkipCompletedSeries(bool val) {
    if (skipCompletedSeries() != val) { m_settings->setValue(QStringLiteral("library/skipCompletedSeries"), val); emit skipCompletedSeriesChanged(); emit settingsChanged(); }
}

// -----------------------------------------------------------------------------
// Storage / Downloads
// -----------------------------------------------------------------------------

QString AppSettings::downloadPath() const { return m_settings->value(QStringLiteral("storage/downloadPath"), AppPaths::defaultDownloadDir()).toString(); }
void AppSettings::setDownloadPath(const QString &val) {
    if (downloadPath() != val) { m_settings->setValue(QStringLiteral("storage/downloadPath"), val); emit downloadPathChanged(); emit settingsChanged(); }
}

int AppSettings::parallelDownloads() const { return m_settings->value(QStringLiteral("storage/parallelDownloads"), 3).toInt(); }
void AppSettings::setParallelDownloads(int val) {
    if (parallelDownloads() != val) { m_settings->setValue(QStringLiteral("storage/parallelDownloads"), val); emit parallelDownloadsChanged(); emit settingsChanged(); }
}

QString AppSettings::downloadFormat() const { return m_settings->value(QStringLiteral("storage/downloadFormat"), QStringLiteral("cbz")).toString(); }
void AppSettings::setDownloadFormat(const QString &val) {
    if (downloadFormat() != val) { m_settings->setValue(QStringLiteral("storage/downloadFormat"), val); emit downloadFormatChanged(); emit settingsChanged(); }
}

// -----------------------------------------------------------------------------
// Sync
// -----------------------------------------------------------------------------

bool AppSettings::syncLibrary() const { return m_settings->value(QStringLiteral("sync/syncLibrary"), true).toBool(); }
void AppSettings::setSyncLibrary(bool val) {
    if (syncLibrary() != val) { m_settings->setValue(QStringLiteral("sync/syncLibrary"), val); emit syncLibraryChanged(); emit settingsChanged(); }
}

bool AppSettings::syncProgress() const { return m_settings->value(QStringLiteral("sync/syncProgress"), true).toBool(); }
void AppSettings::setSyncProgress(bool val) {
    if (syncProgress() != val) { m_settings->setValue(QStringLiteral("sync/syncProgress"), val); emit syncProgressChanged(); emit settingsChanged(); }
}

bool AppSettings::syncSettings() const { return m_settings->value(QStringLiteral("sync/syncSettings"), false).toBool(); }
void AppSettings::setSyncSettings(bool val) {
    if (syncSettings() != val) { m_settings->setValue(QStringLiteral("sync/syncSettings"), val); emit syncSettingsChanged(); emit settingsChanged(); }
}

qint64 AppSettings::lastSyncTime() const { return m_settings->value(QStringLiteral("sync/lastSyncTime"), 0).toLongLong(); }
void AppSettings::setLastSyncTime(qint64 val) {
    if (lastSyncTime() != val) { m_settings->setValue(QStringLiteral("sync/lastSyncTime"), val); emit lastSyncTimeChanged(); emit settingsChanged(); }
}

// -----------------------------------------------------------------------------
// Backup
// -----------------------------------------------------------------------------

QString AppSettings::autoBackupSchedule() const { return m_settings->value(QStringLiteral("backup/autoBackupSchedule"), QStringLiteral("Weekly")).toString(); }
void AppSettings::setAutoBackupSchedule(const QString &val) {
    if (autoBackupSchedule() != val) { m_settings->setValue(QStringLiteral("backup/autoBackupSchedule"), val); emit autoBackupScheduleChanged(); emit settingsChanged(); }
}

QString AppSettings::lastBackupDate() const { return m_settings->value(QStringLiteral("backup/lastBackupDate"), QString()).toString(); }
void AppSettings::setLastBackupDate(const QString &val) {
    if (lastBackupDate() != val) { m_settings->setValue(QStringLiteral("backup/lastBackupDate"), val); emit lastBackupDateChanged(); emit settingsChanged(); }
}

// -----------------------------------------------------------------------------
// Updates
// -----------------------------------------------------------------------------

bool AppSettings::autoCheckUpdates() const { return m_settings->value(QStringLiteral("updates/autoCheckUpdates"), true).toBool(); }
void AppSettings::setAutoCheckUpdates(bool val) {
    if (autoCheckUpdates() != val) { m_settings->setValue(QStringLiteral("updates/autoCheckUpdates"), val); emit autoCheckUpdatesChanged(); emit settingsChanged(); }
}

qint64 AppSettings::lastUpdateCheck() const { return m_settings->value(QStringLiteral("updates/lastUpdateCheck"), 0).toLongLong(); }
void AppSettings::setLastUpdateCheck(qint64 val) {
    if (lastUpdateCheck() != val) { m_settings->setValue(QStringLiteral("updates/lastUpdateCheck"), val); emit lastUpdateCheckChanged(); emit settingsChanged(); }
}

bool AppSettings::installUpdatesAutomatically() const { return m_settings->value(QStringLiteral("updates/installUpdatesAutomatically"), true).toBool(); }
void AppSettings::setInstallUpdatesAutomatically(bool val) {
    if (installUpdatesAutomatically() != val) { m_settings->setValue(QStringLiteral("updates/installUpdatesAutomatically"), val); emit installUpdatesAutomaticallyChanged(); emit settingsChanged(); }
}

// -----------------------------------------------------------------------------
// Trackers
// -----------------------------------------------------------------------------

QString AppSettings::anilistClientId() const { return m_settings->value(QStringLiteral("trackers/anilistClientId"), QString::fromUtf8(kDefaultAnilistClientId)).toString(); }
void AppSettings::setAnilistClientId(const QString &val) {
    if (anilistClientId() != val) { m_settings->setValue(QStringLiteral("trackers/anilistClientId"), val); emit anilistClientIdChanged(); emit settingsChanged(); }
}

QString AppSettings::anilistClientSecret() const { return m_settings->value(QStringLiteral("trackers/anilistClientSecret"), QString::fromUtf8(kDefaultAnilistClientSecret)).toString(); }
void AppSettings::setAnilistClientSecret(const QString &val) {
    if (anilistClientSecret() != val) { m_settings->setValue(QStringLiteral("trackers/anilistClientSecret"), val); emit anilistClientSecretChanged(); emit settingsChanged(); }
}

QString AppSettings::anilistRedirectUri() const { return m_settings->value(QStringLiteral("trackers/anilistRedirectUri"), QString::fromUtf8(kDefaultAnilistRedirectUri)).toString(); }
void AppSettings::setAnilistRedirectUri(const QString &val) {
    if (anilistRedirectUri() != val) { m_settings->setValue(QStringLiteral("trackers/anilistRedirectUri"), val); emit anilistRedirectUriChanged(); emit settingsChanged(); }
}

QString AppSettings::anilistUsername() const { return m_settings->value(QStringLiteral("trackers/anilistUsername"), QString()).toString(); }
void AppSettings::setAnilistUsername(const QString &val) {
    if (anilistUsername() != val) { m_settings->setValue(QStringLiteral("trackers/anilistUsername"), val); emit anilistUsernameChanged(); emit settingsChanged(); }
}

bool AppSettings::anilistConnected() const { return m_settings->value(QStringLiteral("trackers/anilistConnected"), false).toBool(); }
void AppSettings::setAnilistConnected(bool val) {
    if (anilistConnected() != val) { m_settings->setValue(QStringLiteral("trackers/anilistConnected"), val); emit anilistConnectedChanged(); emit settingsChanged(); }
}

QString AppSettings::malClientId() const { return m_settings->value(QStringLiteral("trackers/malClientId"), QString::fromUtf8(kDefaultMalClientId)).toString(); }
void AppSettings::setMalClientId(const QString &val) {
    if (malClientId() != val) { m_settings->setValue(QStringLiteral("trackers/malClientId"), val); emit malClientIdChanged(); emit settingsChanged(); }
}

QString AppSettings::malRedirectUri() const { return m_settings->value(QStringLiteral("trackers/malRedirectUri"), QString::fromUtf8(kDefaultMalRedirectUri)).toString(); }
void AppSettings::setMalRedirectUri(const QString &val) {
    if (malRedirectUri() != val) { m_settings->setValue(QStringLiteral("trackers/malRedirectUri"), val); emit malRedirectUriChanged(); emit settingsChanged(); }
}

QString AppSettings::malUsername() const { return m_settings->value(QStringLiteral("trackers/malUsername"), QString()).toString(); }
void AppSettings::setMalUsername(const QString &val) {
    if (malUsername() != val) { m_settings->setValue(QStringLiteral("trackers/malUsername"), val); emit malUsernameChanged(); emit settingsChanged(); }
}

bool AppSettings::malConnected() const { return m_settings->value(QStringLiteral("trackers/malConnected"), false).toBool(); }
void AppSettings::setMalConnected(bool val) {
    if (malConnected() != val) { m_settings->setValue(QStringLiteral("trackers/malConnected"), val); emit malConnectedChanged(); emit settingsChanged(); }
}

QString AppSettings::googleRedirectUri() const { return m_settings->value(QStringLiteral("trackers/googleRedirectUri"), QString::fromUtf8(kDefaultGoogleRedirectUri)).toString(); }
void AppSettings::setGoogleRedirectUri(const QString &val) {
    if (googleRedirectUri() != val) { m_settings->setValue(QStringLiteral("trackers/googleRedirectUri"), val); emit googleRedirectUriChanged(); emit settingsChanged(); }
}

bool AppSettings::autoSyncTrackers() const { return m_settings->value(QStringLiteral("trackers/autoSyncTrackers"), true).toBool(); }
void AppSettings::setAutoSyncTrackers(bool val) {
    if (autoSyncTrackers() != val) { m_settings->setValue(QStringLiteral("trackers/autoSyncTrackers"), val); emit autoSyncTrackersChanged(); emit settingsChanged(); }
}

bool AppSettings::markTrackerReadingOnFirstChapter() const { return m_settings->value(QStringLiteral("trackers/markTrackerReadingOnFirstChapter"), true).toBool(); }
void AppSettings::setMarkTrackerReadingOnFirstChapter(bool val) {
    if (markTrackerReadingOnFirstChapter() != val) { m_settings->setValue(QStringLiteral("trackers/markTrackerReadingOnFirstChapter"), val); emit markTrackerReadingOnFirstChapterChanged(); emit settingsChanged(); }
}

bool AppSettings::markTrackerCompletedOnFinish() const { return m_settings->value(QStringLiteral("trackers/markTrackerCompletedOnFinish"), true).toBool(); }
void AppSettings::setMarkTrackerCompletedOnFinish(bool val) {
    if (markTrackerCompletedOnFinish() != val) { m_settings->setValue(QStringLiteral("trackers/markTrackerCompletedOnFinish"), val); emit markTrackerCompletedOnFinishChanged(); emit settingsChanged(); }
}

bool AppSettings::pullProgressFromTrackers() const { return m_settings->value(QStringLiteral("trackers/pullProgressFromTrackers"), false).toBool(); }
void AppSettings::setPullProgressFromTrackers(bool val) {
    if (pullProgressFromTrackers() != val) { m_settings->setValue(QStringLiteral("trackers/pullProgressFromTrackers"), val); emit pullProgressFromTrackersChanged(); emit settingsChanged(); }
}

QString AppSettings::kitsuToken() const { return m_settings->value(QStringLiteral("trackers/kitsuToken"), QString()).toString(); }
void AppSettings::setKitsuToken(const QString &val) {
    if (kitsuToken() != val) { m_settings->setValue(QStringLiteral("trackers/kitsuToken"), val); emit kitsuTokenChanged(); emit settingsChanged(); }
}

QString AppSettings::mangaUpdatesToken() const { return m_settings->value(QStringLiteral("trackers/mangaUpdatesToken"), QString()).toString(); }
void AppSettings::setMangaUpdatesToken(const QString &val) {
    if (mangaUpdatesToken() != val) { m_settings->setValue(QStringLiteral("trackers/mangaUpdatesToken"), val); emit mangaUpdatesTokenChanged(); emit settingsChanged(); }
}

QString AppSettings::shikimoriToken() const { return m_settings->value(QStringLiteral("trackers/shikimoriToken"), QString()).toString(); }
void AppSettings::setShikimoriToken(const QString &val) {
    if (shikimoriToken() != val) { m_settings->setValue(QStringLiteral("trackers/shikimoriToken"), val); emit shikimoriTokenChanged(); emit settingsChanged(); }
}

QString AppSettings::bangumiToken() const { return m_settings->value(QStringLiteral("trackers/bangumiToken"), QString()).toString(); }
void AppSettings::setBangumiToken(const QString &val) {
    if (bangumiToken() != val) { m_settings->setValue(QStringLiteral("trackers/bangumiToken"), val); emit bangumiTokenChanged(); emit settingsChanged(); }
}

// Profile & Gamification Goals
QStringList AppSettings::pinnedBadges() const { return m_settings->value(QStringLiteral("profile/pinnedBadges"), QStringList()).toStringList(); }
void AppSettings::setPinnedBadges(const QStringList &val) {
    if (pinnedBadges() != val) { m_settings->setValue(QStringLiteral("profile/pinnedBadges"), val); emit pinnedBadgesChanged(); emit settingsChanged(); }
}

int AppSettings::monthlyChapterGoal() const { return m_settings->value(QStringLiteral("profile/monthlyChapterGoal"), 40).toInt(); }
void AppSettings::setMonthlyChapterGoal(int val) {
    if (monthlyChapterGoal() != val) { m_settings->setValue(QStringLiteral("profile/monthlyChapterGoal"), val); emit monthlyChapterGoalChanged(); emit settingsChanged(); }
}

int AppSettings::yearlyMangaGoal() const { return m_settings->value(QStringLiteral("profile/yearlyMangaGoal"), 25).toInt(); }
void AppSettings::setYearlyMangaGoal(int val) {
    if (yearlyMangaGoal() != val) { m_settings->setValue(QStringLiteral("profile/yearlyMangaGoal"), val); emit yearlyMangaGoalChanged(); emit settingsChanged(); }
}

// Sources & Search Filter
QStringList AppSettings::enabledSearchSources() const { return m_settings->value(QStringLiteral("sources/enabledSearchSources"), QStringList()).toStringList(); }
void AppSettings::setEnabledSearchSources(const QStringList &val) {
    if (enabledSearchSources() != val) { m_settings->setValue(QStringLiteral("sources/enabledSearchSources"), val); emit enabledSearchSourcesChanged(); emit settingsChanged(); }
}

QStringList AppSettings::disabledExtensions() const { return m_settings->value(QStringLiteral("sources/disabledExtensions"), QStringList()).toStringList(); }
void AppSettings::setDisabledExtensions(const QStringList &val) {
    if (disabledExtensions() != val) { m_settings->setValue(QStringLiteral("sources/disabledExtensions"), val); emit disabledExtensionsChanged(); emit settingsChanged(); }
}

QStringList AppSettings::customExtensionRepos() const { return m_settings->value(QStringLiteral("sources/customExtensionRepos"), QStringList()).toStringList(); }
void AppSettings::setCustomExtensionRepos(const QStringList &val) {
    if (customExtensionRepos() != val) { m_settings->setValue(QStringLiteral("sources/customExtensionRepos"), val); emit customExtensionReposChanged(); emit settingsChanged(); }
}

// Security & App Lock
bool AppSettings::appLockEnabled() const { return m_settings->value(QStringLiteral("security/appLockEnabled"), false).toBool(); }
void AppSettings::setAppLockEnabled(bool val) {
    if (appLockEnabled() != val) { m_settings->setValue(QStringLiteral("security/appLockEnabled"), val); emit appLockEnabledChanged(); emit settingsChanged(); }
}

QString AppSettings::appLockPin() const { return m_settings->value(QStringLiteral("security/appLockPin"), QString()).toString(); }
void AppSettings::setAppLockPin(const QString &val) {
    if (appLockPin() != val) { m_settings->setValue(QStringLiteral("security/appLockPin"), val); emit appLockPinChanged(); emit settingsChanged(); }
}

// Media Servers (Komga & Suwayomi)
bool AppSettings::komgaEnabled() const { return m_settings->value(QStringLiteral("servers/komgaEnabled"), false).toBool(); }
void AppSettings::setKomgaEnabled(bool val) {
    if (komgaEnabled() != val) { m_settings->setValue(QStringLiteral("servers/komgaEnabled"), val); emit komgaEnabledChanged(); emit settingsChanged(); }
}

QString AppSettings::komgaUrl() const { return m_settings->value(QStringLiteral("servers/komgaUrl"), QString()).toString(); }
void AppSettings::setKomgaUrl(const QString &val) {
    if (komgaUrl() != val) { m_settings->setValue(QStringLiteral("servers/komgaUrl"), val); emit komgaUrlChanged(); emit settingsChanged(); }
}

QString AppSettings::komgaUsername() const { return m_settings->value(QStringLiteral("servers/komgaUsername"), QString()).toString(); }
void AppSettings::setKomgaUsername(const QString &val) {
    if (komgaUsername() != val) { m_settings->setValue(QStringLiteral("servers/komgaUsername"), val); emit komgaUsernameChanged(); emit settingsChanged(); }
}

QString AppSettings::komgaPassword() const { return m_settings->value(QStringLiteral("servers/komgaPassword"), QString()).toString(); }
void AppSettings::setKomgaPassword(const QString &val) {
    if (komgaPassword() != val) { m_settings->setValue(QStringLiteral("servers/komgaPassword"), val); emit komgaPasswordChanged(); emit settingsChanged(); }
}

bool AppSettings::suwayomiEnabled() const { return m_settings->value(QStringLiteral("servers/suwayomiEnabled"), false).toBool(); }
void AppSettings::setSuwayomiEnabled(bool val) {
    if (suwayomiEnabled() != val) { m_settings->setValue(QStringLiteral("servers/suwayomiEnabled"), val); emit suwayomiEnabledChanged(); emit settingsChanged(); }
}

QString AppSettings::suwayomiUrl() const { return m_settings->value(QStringLiteral("servers/suwayomiUrl"), QString()).toString(); }
void AppSettings::setSuwayomiUrl(const QString &val) {
    if (suwayomiUrl() != val) { m_settings->setValue(QStringLiteral("servers/suwayomiUrl"), val); emit suwayomiUrlChanged(); emit settingsChanged(); }
}

QString AppSettings::suwayomiApiKey() const { return m_settings->value(QStringLiteral("servers/suwayomiApiKey"), QString()).toString(); }
void AppSettings::setSuwayomiApiKey(const QString &val) {
    if (suwayomiApiKey() != val) { m_settings->setValue(QStringLiteral("servers/suwayomiApiKey"), val); emit suwayomiApiKeyChanged(); emit settingsChanged(); }
}

// Onboarding & Tours
bool AppSettings::onboardingCompleted() const { return m_settings->value(QStringLiteral("app/onboardingCompleted"), false).toBool(); }
void AppSettings::setOnboardingCompleted(bool val) {
    if (onboardingCompleted() != val) { m_settings->setValue(QStringLiteral("app/onboardingCompleted"), val); emit onboardingCompletedChanged(); emit settingsChanged(); }
}

bool AppSettings::libraryTourCompleted() const { return m_settings->value(QStringLiteral("app/libraryTourCompleted"), false).toBool(); }
void AppSettings::setLibraryTourCompleted(bool val) {
    if (libraryTourCompleted() != val) { m_settings->setValue(QStringLiteral("app/libraryTourCompleted"), val); emit libraryTourCompletedChanged(); emit settingsChanged(); }
}

bool AppSettings::settingsTourCompleted() const { return m_settings->value(QStringLiteral("app/settingsTourCompleted"), false).toBool(); }
void AppSettings::setSettingsTourCompleted(bool val) {
    if (settingsTourCompleted() != val) { m_settings->setValue(QStringLiteral("app/settingsTourCompleted"), val); emit settingsTourCompletedChanged(); emit settingsChanged(); }
}

QString AppSettings::authSecret(const QString &key) const
{
    return m_settings->value(QStringLiteral("auth/secret_") + key).toString();
}

void AppSettings::setAuthSecret(const QString &key, const QString &val)
{
    m_settings->setValue(QStringLiteral("auth/secret_") + key, val);
}

void AppSettings::removeAuthSecret(const QString &key)
{
    m_settings->remove(QStringLiteral("auth/secret_") + key);
}

} // namespace mangadl
