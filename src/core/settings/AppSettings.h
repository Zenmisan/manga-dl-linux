#pragma once

#include <QObject>
#include <QString>
#include <QSettings>
#include <memory>

namespace mangadl {

class AppSettings : public QObject {
    Q_OBJECT

    // Appearance
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QString accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    Q_PROPERTY(bool ambilight READ ambilight WRITE setAmbilight NOTIFY ambilightChanged)

    // Connection
    Q_PROPERTY(QString backendUrl READ backendUrl WRITE setBackendUrl NOTIFY backendUrlChanged)
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString supabaseUrl READ supabaseUrl WRITE setSupabaseUrl NOTIFY supabaseUrlChanged)
    Q_PROPERTY(QString supabaseAnonKey READ supabaseAnonKey WRITE setSupabaseAnonKey NOTIFY supabaseAnonKeyChanged)
    Q_PROPERTY(bool hasSupabase READ hasSupabase NOTIFY supabaseUrlChanged)

    // Account
    Q_PROPERTY(QString userEmail READ userEmail WRITE setUserEmail NOTIFY userEmailChanged)
    Q_PROPERTY(QString userId READ userId WRITE setUserId NOTIFY userIdChanged)
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged)
    Q_PROPERTY(QString bio READ bio WRITE setBio NOTIFY bioChanged)
    Q_PROPERTY(QString avatarUrl READ avatarUrl WRITE setAvatarUrl NOTIFY avatarUrlChanged)
    Q_PROPERTY(bool showReadingActivity READ showReadingActivity WRITE setShowReadingActivity NOTIFY showReadingActivityChanged)

    // Desktop
    Q_PROPERTY(bool launchOnStartup READ launchOnStartup WRITE setLaunchOnStartup NOTIFY launchOnStartupChanged)
    Q_PROPERTY(bool keepRunningInTray READ keepRunningInTray WRITE setKeepRunningInTray NOTIFY keepRunningInTrayChanged)
    Q_PROPERTY(bool desktopNotifications READ desktopNotifications WRITE setDesktopNotifications NOTIFY desktopNotificationsChanged)
    Q_PROPERTY(bool discordRpc READ discordRpc WRITE setDiscordRpc NOTIFY discordRpcChanged)

    // Behaviour
    Q_PROPERTY(bool incognito READ incognito WRITE setIncognito NOTIFY incognitoChanged)
    Q_PROPERTY(bool autoDownloadNew READ autoDownloadNew WRITE setAutoDownloadNew NOTIFY autoDownloadNewChanged)

    // Reader
    Q_PROPERTY(QString readerMode READ readerMode WRITE setReaderMode NOTIFY readerModeChanged)
    Q_PROPERTY(QString readingDirection READ readingDirection WRITE setReadingDirection NOTIFY readingDirectionChanged)
    Q_PROPERTY(bool twoPageSpread READ twoPageSpread WRITE setTwoPageSpread NOTIFY twoPageSpreadChanged)
    Q_PROPERTY(bool cropBorders READ cropBorders WRITE setCropBorders NOTIFY cropBordersChanged)
    Q_PROPERTY(int webtoonSidePadding READ webtoonSidePadding WRITE setWebtoonSidePadding NOTIFY webtoonSidePaddingChanged)
    Q_PROPERTY(bool clickZones READ clickZones WRITE setClickZones NOTIFY clickZonesChanged)
    Q_PROPERTY(QString mouseWheelAction READ mouseWheelAction WRITE setMouseWheelAction NOTIFY mouseWheelActionChanged)
    Q_PROPERTY(QString pageFit READ pageFit WRITE setPageFit NOTIFY pageFitChanged)
    Q_PROPERTY(QString readerBackground READ readerBackground WRITE setReaderBackground NOTIFY readerBackgroundChanged)
    Q_PROPERTY(bool showPageNumber READ showPageNumber WRITE setShowPageNumber NOTIFY showPageNumberChanged)
    Q_PROPERTY(bool openFullscreen READ openFullscreen WRITE setOpenFullscreen NOTIFY openFullscreenChanged)
    Q_PROPERTY(int imageBrightness READ imageBrightness WRITE setImageBrightness NOTIFY imageBrightnessChanged)
    Q_PROPERTY(bool imageGrayscale READ imageGrayscale WRITE setImageGrayscale NOTIFY imageGrayscaleChanged)
    Q_PROPERTY(bool imageInvert READ imageInvert WRITE setImageInvert NOTIFY imageInvertChanged)
    Q_PROPERTY(bool sharpenImages READ sharpenImages WRITE setSharpenImages NOTIFY sharpenImagesChanged)

    // Novel Reader
    Q_PROPERTY(int novelFontSize READ novelFontSize WRITE setNovelFontSize NOTIFY novelFontSizeChanged)
    Q_PROPERTY(QString novelFontFamily READ novelFontFamily WRITE setNovelFontFamily NOTIFY novelFontFamilyChanged)
    Q_PROPERTY(double novelLineHeight READ novelLineHeight WRITE setNovelLineHeight NOTIFY novelLineHeightChanged)
    Q_PROPERTY(QString novelTheme READ novelTheme WRITE setNovelTheme NOTIFY novelThemeChanged)

    // Library
    Q_PROPERTY(QString gridColumns READ gridColumns WRITE setGridColumns NOTIFY gridColumnsChanged)
    Q_PROPERTY(QString libraryViewMode READ libraryViewMode WRITE setLibraryViewMode NOTIFY libraryViewModeChanged)
    Q_PROPERTY(QString libraryDensity READ libraryDensity WRITE setLibraryDensity NOTIFY libraryDensityChanged)
    Q_PROPERTY(QString libraryContentType READ libraryContentType WRITE setLibraryContentType NOTIFY libraryContentTypeChanged)
    Q_PROPERTY(QString libraryFilterMode READ libraryFilterMode WRITE setLibraryFilterMode NOTIFY libraryFilterModeChanged)
    Q_PROPERTY(QString librarySortMode READ librarySortMode WRITE setLibrarySortMode NOTIFY librarySortModeChanged)
    Q_PROPERTY(bool unreadBadges READ unreadBadges WRITE setUnreadBadges NOTIFY unreadBadgesChanged)
    Q_PROPERTY(bool downloadedBadges READ downloadedBadges WRITE setDownloadedBadges NOTIFY downloadedBadgesChanged)
    Q_PROPERTY(bool continueReadingRow READ continueReadingRow WRITE setContinueReadingRow NOTIFY continueReadingRowChanged)
    Q_PROPERTY(QString checkNewChapters READ checkNewChapters WRITE setCheckNewChapters NOTIFY checkNewChaptersChanged)
    Q_PROPERTY(bool onlyUnmetered READ onlyUnmetered WRITE setOnlyUnmetered NOTIFY onlyUnmeteredChanged)
    Q_PROPERTY(bool skipCompletedSeries READ skipCompletedSeries WRITE setSkipCompletedSeries NOTIFY skipCompletedSeriesChanged)

    // Storage / Downloads
    Q_PROPERTY(QString downloadPath READ downloadPath WRITE setDownloadPath NOTIFY downloadPathChanged)
    Q_PROPERTY(int parallelDownloads READ parallelDownloads WRITE setParallelDownloads NOTIFY parallelDownloadsChanged)
    Q_PROPERTY(QString downloadFormat READ downloadFormat WRITE setDownloadFormat NOTIFY downloadFormatChanged)

    // Sync
    Q_PROPERTY(bool syncLibrary READ syncLibrary WRITE setSyncLibrary NOTIFY syncLibraryChanged)
    Q_PROPERTY(bool syncProgress READ syncProgress WRITE setSyncProgress NOTIFY syncProgressChanged)
    Q_PROPERTY(bool syncSettings READ syncSettings WRITE setSyncSettings NOTIFY syncSettingsChanged)
    Q_PROPERTY(qint64 lastSyncTime READ lastSyncTime WRITE setLastSyncTime NOTIFY lastSyncTimeChanged)

    // Backup
    Q_PROPERTY(QString autoBackupSchedule READ autoBackupSchedule WRITE setAutoBackupSchedule NOTIFY autoBackupScheduleChanged)
    Q_PROPERTY(QString lastBackupDate READ lastBackupDate WRITE setLastBackupDate NOTIFY lastBackupDateChanged)

    // Updates
    Q_PROPERTY(bool autoCheckUpdates READ autoCheckUpdates WRITE setAutoCheckUpdates NOTIFY autoCheckUpdatesChanged)
    Q_PROPERTY(qint64 lastUpdateCheck READ lastUpdateCheck WRITE setLastUpdateCheck NOTIFY lastUpdateCheckChanged)
    Q_PROPERTY(bool installUpdatesAutomatically READ installUpdatesAutomatically WRITE setInstallUpdatesAutomatically NOTIFY installUpdatesAutomaticallyChanged)

    // Trackers & Integrations
    Q_PROPERTY(QString anilistClientId READ anilistClientId WRITE setAnilistClientId NOTIFY anilistClientIdChanged)
    Q_PROPERTY(QString anilistClientSecret READ anilistClientSecret WRITE setAnilistClientSecret NOTIFY anilistClientSecretChanged)
    Q_PROPERTY(QString anilistRedirectUri READ anilistRedirectUri WRITE setAnilistRedirectUri NOTIFY anilistRedirectUriChanged)
    Q_PROPERTY(QString anilistUsername READ anilistUsername WRITE setAnilistUsername NOTIFY anilistUsernameChanged)
    Q_PROPERTY(bool anilistConnected READ anilistConnected WRITE setAnilistConnected NOTIFY anilistConnectedChanged)
    Q_PROPERTY(QString malClientId READ malClientId WRITE setMalClientId NOTIFY malClientIdChanged)
    Q_PROPERTY(QString malRedirectUri READ malRedirectUri WRITE setMalRedirectUri NOTIFY malRedirectUriChanged)
    Q_PROPERTY(QString malUsername READ malUsername WRITE setMalUsername NOTIFY malUsernameChanged)
    Q_PROPERTY(bool malConnected READ malConnected WRITE setMalConnected NOTIFY malConnectedChanged)
    Q_PROPERTY(QString googleRedirectUri READ googleRedirectUri WRITE setGoogleRedirectUri NOTIFY googleRedirectUriChanged)
    Q_PROPERTY(QString kitsuToken READ kitsuToken WRITE setKitsuToken NOTIFY kitsuTokenChanged)
    Q_PROPERTY(QString mangaUpdatesToken READ mangaUpdatesToken WRITE setMangaUpdatesToken NOTIFY mangaUpdatesTokenChanged)
    Q_PROPERTY(QString shikimoriToken READ shikimoriToken WRITE setShikimoriToken NOTIFY shikimoriTokenChanged)
    Q_PROPERTY(QString bangumiToken READ bangumiToken WRITE setBangumiToken NOTIFY bangumiTokenChanged)
    Q_PROPERTY(bool autoSyncTrackers READ autoSyncTrackers WRITE setAutoSyncTrackers NOTIFY autoSyncTrackersChanged)
    Q_PROPERTY(bool markTrackerReadingOnFirstChapter READ markTrackerReadingOnFirstChapter WRITE setMarkTrackerReadingOnFirstChapter NOTIFY markTrackerReadingOnFirstChapterChanged)
    Q_PROPERTY(bool markTrackerCompletedOnFinish READ markTrackerCompletedOnFinish WRITE setMarkTrackerCompletedOnFinish NOTIFY markTrackerCompletedOnFinishChanged)
    Q_PROPERTY(bool pullProgressFromTrackers READ pullProgressFromTrackers WRITE setPullProgressFromTrackers NOTIFY pullProgressFromTrackersChanged)

    // Profile & Gamification Goals
    Q_PROPERTY(QStringList pinnedBadges READ pinnedBadges WRITE setPinnedBadges NOTIFY pinnedBadgesChanged)
    Q_PROPERTY(int monthlyChapterGoal READ monthlyChapterGoal WRITE setMonthlyChapterGoal NOTIFY monthlyChapterGoalChanged)
    Q_PROPERTY(int yearlyMangaGoal READ yearlyMangaGoal WRITE setYearlyMangaGoal NOTIFY yearlyMangaGoalChanged)

    // Sources & Search Filter
    Q_PROPERTY(QStringList enabledSearchSources READ enabledSearchSources WRITE setEnabledSearchSources NOTIFY enabledSearchSourcesChanged)
    Q_PROPERTY(QStringList disabledExtensions READ disabledExtensions WRITE setDisabledExtensions NOTIFY disabledExtensionsChanged)
    Q_PROPERTY(QStringList customExtensionRepos READ customExtensionRepos WRITE setCustomExtensionRepos NOTIFY customExtensionReposChanged)

    // Security & App Lock
    Q_PROPERTY(bool appLockEnabled READ appLockEnabled WRITE setAppLockEnabled NOTIFY appLockEnabledChanged)
    Q_PROPERTY(QString appLockPin READ appLockPin WRITE setAppLockPin NOTIFY appLockPinChanged)

    // Media Servers (Komga & Suwayomi)
    Q_PROPERTY(bool komgaEnabled READ komgaEnabled WRITE setKomgaEnabled NOTIFY komgaEnabledChanged)
    Q_PROPERTY(QString komgaUrl READ komgaUrl WRITE setKomgaUrl NOTIFY komgaUrlChanged)
    Q_PROPERTY(QString komgaUsername READ komgaUsername WRITE setKomgaUsername NOTIFY komgaUsernameChanged)
    Q_PROPERTY(QString komgaPassword READ komgaPassword WRITE setKomgaPassword NOTIFY komgaPasswordChanged)
    Q_PROPERTY(bool suwayomiEnabled READ suwayomiEnabled WRITE setSuwayomiEnabled NOTIFY suwayomiEnabledChanged)
    Q_PROPERTY(QString suwayomiUrl READ suwayomiUrl WRITE setSuwayomiUrl NOTIFY suwayomiUrlChanged)
    Q_PROPERTY(QString suwayomiApiKey READ suwayomiApiKey WRITE setSuwayomiApiKey NOTIFY suwayomiApiKeyChanged)

    // Onboarding & Quick Tours
    Q_PROPERTY(bool onboardingCompleted READ onboardingCompleted WRITE setOnboardingCompleted NOTIFY onboardingCompletedChanged)
    Q_PROPERTY(bool libraryTourCompleted READ libraryTourCompleted WRITE setLibraryTourCompleted NOTIFY libraryTourCompletedChanged)
    Q_PROPERTY(bool settingsTourCompleted READ settingsTourCompleted WRITE setSettingsTourCompleted NOTIFY settingsTourCompletedChanged)

public:
    explicit AppSettings(const QString &configFilePath = QString(), QObject *parent = nullptr);
    ~AppSettings() override;

    static AppSettings *instance();
    static void setInstance(AppSettings *customInstance);

    // Appearance
    QString theme() const;
    void setTheme(const QString &val);
    QString accentColor() const;
    void setAccentColor(const QString &val);
    bool ambilight() const;
    void setAmbilight(bool val);

    // Connection
    QString backendUrl() const;
    void setBackendUrl(const QString &val);
    QString apiKey() const;
    void setApiKey(const QString &val);
    QString supabaseUrl() const;
    void setSupabaseUrl(const QString &val);
    QString supabaseAnonKey() const;
    void setSupabaseAnonKey(const QString &val);
    bool hasSupabase() const;

    // Account
    QString userEmail() const;
    void setUserEmail(const QString &val);
    QString userId() const;
    void setUserId(const QString &val);
    QString displayName() const;
    void setDisplayName(const QString &val);
    QString bio() const;
    void setBio(const QString &val);
    QString avatarUrl() const;
    void setAvatarUrl(const QString &val);
    bool showReadingActivity() const;
    void setShowReadingActivity(bool val);

    // Desktop
    bool launchOnStartup() const;
    void setLaunchOnStartup(bool val);
    bool keepRunningInTray() const;
    void setKeepRunningInTray(bool val);
    bool desktopNotifications() const;
    void setDesktopNotifications(bool val);
    bool discordRpc() const;
    void setDiscordRpc(bool val);

    // Behaviour
    bool incognito() const;
    void setIncognito(bool val);
    bool autoDownloadNew() const;
    void setAutoDownloadNew(bool val);

    // Reader
    QString readerMode() const;
    void setReaderMode(const QString &val);
    QString readingDirection() const;
    void setReadingDirection(const QString &val);
    bool twoPageSpread() const;
    void setTwoPageSpread(bool val);
    bool cropBorders() const;
    void setCropBorders(bool val);
    int webtoonSidePadding() const;
    void setWebtoonSidePadding(int val);
    bool clickZones() const;
    void setClickZones(bool val);
    QString mouseWheelAction() const;
    void setMouseWheelAction(const QString &val);
    QString pageFit() const;
    void setPageFit(const QString &val);
    QString readerBackground() const;
    void setReaderBackground(const QString &val);
    bool showPageNumber() const;
    void setShowPageNumber(bool val);
    bool openFullscreen() const;
    void setOpenFullscreen(bool val);
    int imageBrightness() const;
    void setImageBrightness(int val);
    bool imageGrayscale() const;
    void setImageGrayscale(bool val);
    bool imageInvert() const;
    void setImageInvert(bool val);
    bool sharpenImages() const;
    void setSharpenImages(bool val);

    // Novel Reader
    int novelFontSize() const;
    void setNovelFontSize(int val);
    QString novelFontFamily() const;
    void setNovelFontFamily(const QString &val);
    double novelLineHeight() const;
    void setNovelLineHeight(double val);
    QString novelTheme() const;
    void setNovelTheme(const QString &val);

    // Library
    QString gridColumns() const;
    void setGridColumns(const QString &val);
    QString libraryViewMode() const;
    void setLibraryViewMode(const QString &val);
    QString libraryDensity() const;
    void setLibraryDensity(const QString &val);
    QString libraryContentType() const;
    void setLibraryContentType(const QString &val);
    QString libraryFilterMode() const;
    void setLibraryFilterMode(const QString &val);
    QString librarySortMode() const;
    void setLibrarySortMode(const QString &val);
    bool unreadBadges() const;
    void setUnreadBadges(bool val);
    bool downloadedBadges() const;
    void setDownloadedBadges(bool val);
    bool continueReadingRow() const;
    void setContinueReadingRow(bool val);
    QString checkNewChapters() const;
    void setCheckNewChapters(const QString &val);
    bool onlyUnmetered() const;
    void setOnlyUnmetered(bool val);
    bool skipCompletedSeries() const;
    void setSkipCompletedSeries(bool val);

    // Storage / Downloads
    QString downloadPath() const;
    void setDownloadPath(const QString &val);
    int parallelDownloads() const;
    void setParallelDownloads(int val);
    QString downloadFormat() const;
    void setDownloadFormat(const QString &val);

    // Sync
    bool syncLibrary() const;
    void setSyncLibrary(bool val);
    bool syncProgress() const;
    void setSyncProgress(bool val);
    bool syncSettings() const;
    void setSyncSettings(bool val);
    qint64 lastSyncTime() const;
    void setLastSyncTime(qint64 val);

    // Backup
    QString autoBackupSchedule() const;
    void setAutoBackupSchedule(const QString &val);
    QString lastBackupDate() const;
    void setLastBackupDate(const QString &val);

    // Updates
    bool autoCheckUpdates() const;
    void setAutoCheckUpdates(bool val);
    qint64 lastUpdateCheck() const;
    void setLastUpdateCheck(qint64 val);
    bool installUpdatesAutomatically() const;
    void setInstallUpdatesAutomatically(bool val);

    // Trackers
    QString anilistClientId() const;
    void setAnilistClientId(const QString &val);
    QString anilistClientSecret() const;
    void setAnilistClientSecret(const QString &val);
    QString anilistRedirectUri() const;
    void setAnilistRedirectUri(const QString &val);
    QString anilistUsername() const;
    void setAnilistUsername(const QString &val);
    bool anilistConnected() const;
    void setAnilistConnected(bool val);

    QString malClientId() const;
    void setMalClientId(const QString &val);
    QString malRedirectUri() const;
    void setMalRedirectUri(const QString &val);
    QString malUsername() const;
    void setMalUsername(const QString &val);
    bool malConnected() const;
    void setMalConnected(bool val);

    QString googleRedirectUri() const;
    void setGoogleRedirectUri(const QString &val);

    QString kitsuToken() const;
    void setKitsuToken(const QString &val);
    QString mangaUpdatesToken() const;
    void setMangaUpdatesToken(const QString &val);
    QString shikimoriToken() const;
    void setShikimoriToken(const QString &val);
    QString bangumiToken() const;
    void setBangumiToken(const QString &val);

    bool autoSyncTrackers() const;
    void setAutoSyncTrackers(bool val);
    bool markTrackerReadingOnFirstChapter() const;
    void setMarkTrackerReadingOnFirstChapter(bool val);
    bool markTrackerCompletedOnFinish() const;
    void setMarkTrackerCompletedOnFinish(bool val);
    bool pullProgressFromTrackers() const;
    void setPullProgressFromTrackers(bool val);

    // Profile & Gamification Goals
    QStringList pinnedBadges() const;
    void setPinnedBadges(const QStringList &val);
    int monthlyChapterGoal() const;
    void setMonthlyChapterGoal(int val);
    int yearlyMangaGoal() const;
    void setYearlyMangaGoal(int val);

    // Sources & Extensions
    QStringList enabledSearchSources() const;
    void setEnabledSearchSources(const QStringList &val);
    QStringList disabledExtensions() const;
    void setDisabledExtensions(const QStringList &val);
    QStringList customExtensionRepos() const;
    void setCustomExtensionRepos(const QStringList &val);

    // Security & App Lock
    bool appLockEnabled() const;
    void setAppLockEnabled(bool val);
    QString appLockPin() const;
    void setAppLockPin(const QString &val);

    // Media Servers
    bool komgaEnabled() const;
    void setKomgaEnabled(bool val);
    QString komgaUrl() const;
    void setKomgaUrl(const QString &val);
    QString komgaUsername() const;
    void setKomgaUsername(const QString &val);
    QString komgaPassword() const;
    void setKomgaPassword(const QString &val);
    bool suwayomiEnabled() const;
    void setSuwayomiEnabled(bool val);
    QString suwayomiUrl() const;
    void setSuwayomiUrl(const QString &val);
    QString suwayomiApiKey() const;
    void setSuwayomiApiKey(const QString &val);

    // Onboarding & Tours
    bool onboardingCompleted() const;
    void setOnboardingCompleted(bool val);
    bool libraryTourCompleted() const;
    void setLibraryTourCompleted(bool val);
    bool settingsTourCompleted() const;
    void setSettingsTourCompleted(bool val);

    Q_INVOKABLE void resetToDefaults();
    Q_INVOKABLE void sync();

    QString authSecret(const QString &key) const;
    void setAuthSecret(const QString &key, const QString &val);
    void removeAuthSecret(const QString &key);

signals:
    void settingsChanged();
    void themeChanged();
    void accentColorChanged();
    void ambilightChanged();
    void backendUrlChanged();
    void apiKeyChanged();
    void supabaseUrlChanged();
    void supabaseAnonKeyChanged();
    void userEmailChanged();
    void userIdChanged();
    void displayNameChanged();
    void bioChanged();
    void avatarUrlChanged();
    void showReadingActivityChanged();
    void launchOnStartupChanged();
    void keepRunningInTrayChanged();
    void desktopNotificationsChanged();
    void discordRpcChanged();
    void incognitoChanged();
    void autoDownloadNewChanged();
    void readerModeChanged();
    void readingDirectionChanged();
    void twoPageSpreadChanged();
    void cropBordersChanged();
    void webtoonSidePaddingChanged();
    void clickZonesChanged();
    void mouseWheelActionChanged();
    void pageFitChanged();
    void readerBackgroundChanged();
    void showPageNumberChanged();
    void openFullscreenChanged();
    void imageBrightnessChanged();
    void imageGrayscaleChanged();
    void imageInvertChanged();
    void sharpenImagesChanged();
    void novelFontSizeChanged();
    void novelFontFamilyChanged();
    void novelLineHeightChanged();
    void novelThemeChanged();
    void gridColumnsChanged();
    void libraryViewModeChanged();
    void libraryDensityChanged();
    void libraryContentTypeChanged();
    void libraryFilterModeChanged();
    void librarySortModeChanged();
    void unreadBadgesChanged();
    void downloadedBadgesChanged();
    void continueReadingRowChanged();
    void checkNewChaptersChanged();
    void onlyUnmeteredChanged();
    void skipCompletedSeriesChanged();
    void downloadPathChanged();
    void parallelDownloadsChanged();
    void downloadFormatChanged();
    void syncLibraryChanged();
    void syncProgressChanged();
    void syncSettingsChanged();
    void lastSyncTimeChanged();
    void autoBackupScheduleChanged();
    void lastBackupDateChanged();
    void autoCheckUpdatesChanged();
    void lastUpdateCheckChanged();
    void installUpdatesAutomaticallyChanged();
    void anilistClientIdChanged();
    void anilistClientSecretChanged();
    void anilistRedirectUriChanged();
    void anilistUsernameChanged();
    void anilistConnectedChanged();
    void malClientIdChanged();
    void malRedirectUriChanged();
    void malUsernameChanged();
    void malConnectedChanged();
    void googleRedirectUriChanged();
    void kitsuTokenChanged();
    void mangaUpdatesTokenChanged();
    void shikimoriTokenChanged();
    void bangumiTokenChanged();
    void autoSyncTrackersChanged();
    void markTrackerReadingOnFirstChapterChanged();
    void markTrackerCompletedOnFinishChanged();
    void pullProgressFromTrackersChanged();
    void pinnedBadgesChanged();
    void monthlyChapterGoalChanged();
    void yearlyMangaGoalChanged();
    void enabledSearchSourcesChanged();
    void disabledExtensionsChanged();
    void customExtensionReposChanged();
    void appLockEnabledChanged();
    void appLockPinChanged();
    void komgaEnabledChanged();
    void komgaUrlChanged();
    void komgaUsernameChanged();
    void komgaPasswordChanged();
    void suwayomiEnabledChanged();
    void suwayomiUrlChanged();
    void suwayomiApiKeyChanged();
    void onboardingCompletedChanged();
    void libraryTourCompletedChanged();
    void settingsTourCompletedChanged();

private:
    std::unique_ptr<QSettings> m_settings;
};

} // namespace mangadl
