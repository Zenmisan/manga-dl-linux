#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>
#include <QJSEngine>

namespace mangadl {

class MangaDatabase;

class ProfileController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString displayName READ displayName NOTIFY profileChanged)
    Q_PROPERTY(QString username READ username NOTIFY profileChanged)
    Q_PROPERTY(QString bio READ bio NOTIFY profileChanged)
    Q_PROPERTY(QString readingSince READ readingSince NOTIFY profileChanged)

    Q_PROPERTY(int chaptersRead READ chaptersRead NOTIFY profileChanged)
    Q_PROPERTY(int libraryCount READ libraryCount NOTIFY profileChanged)
    Q_PROPERTY(int currentStreak READ currentStreak NOTIFY profileChanged)

    Q_PROPERTY(QString rankName READ rankName NOTIFY profileChanged)
    Q_PROPERTY(QString rankTag READ rankTag NOTIFY profileChanged)
    Q_PROPERTY(QString rankTier READ rankTier NOTIFY profileChanged)
    Q_PROPERTY(QString rankCode READ rankCode NOTIFY profileChanged)
    Q_PROPERTY(QString rankColor READ rankColor NOTIFY profileChanged)
    Q_PROPERTY(QString rankBgColor READ rankBgColor NOTIFY profileChanged)
    Q_PROPERTY(int score READ score NOTIFY profileChanged)
    Q_PROPERTY(double progressToNextRank READ progressToNextRank NOTIFY profileChanged)
    Q_PROPERTY(QString nextRankInfo READ nextRankInfo NOTIFY profileChanged)

    Q_PROPERTY(int unlockedMilestonesCount READ unlockedMilestonesCount NOTIFY profileChanged)
    Q_PROPERTY(int totalMilestonesCount READ totalMilestonesCount NOTIFY profileChanged)
    Q_PROPERTY(QVariantList unlockedBadges READ unlockedBadges NOTIFY profileChanged)
    Q_PROPERTY(QVariantList lockedBadges READ lockedBadges NOTIFY profileChanged)
    Q_PROPERTY(QVariantList recentActivity READ recentActivity NOTIFY profileChanged)
    Q_PROPERTY(QVariantList pinnedCovers READ pinnedCovers NOTIFY profileChanged)

public:
    explicit ProfileController(MangaDatabase *db = nullptr, QObject *parent = nullptr);
    ~ProfileController() override = default;

    static ProfileController &instance();
    static ProfileController *create(QQmlEngine *engine, QJSEngine *scriptEngine);

    QString displayName() const { return m_displayName; }
    QString username() const { return m_username; }
    QString bio() const { return m_bio; }
    QString readingSince() const { return m_readingSince; }

    int chaptersRead() const { return m_chaptersRead; }
    int libraryCount() const { return m_libraryCount; }
    int currentStreak() const { return m_currentStreak; }

    QString rankName() const { return m_rankName; }
    QString rankTag() const { return m_rankTag; }
    QString rankTier() const { return m_rankTier; }
    QString rankCode() const { return m_rankCode; }
    QString rankColor() const { return m_rankColor; }
    QString rankBgColor() const { return m_rankBgColor; }
    int score() const { return m_score; }
    double progressToNextRank() const { return m_progressToNextRank; }
    QString nextRankInfo() const { return m_nextRankInfo; }

    int unlockedMilestonesCount() const { return m_unlockedMilestonesCount; }
    int totalMilestonesCount() const { return m_totalMilestonesCount; }
    QVariantList unlockedBadges() const { return m_unlockedBadges; }
    QVariantList lockedBadges() const { return m_lockedBadges; }
    QVariantList recentActivity() const { return m_recentActivity; }
    QVariantList pinnedCovers() const { return m_pinnedCovers; }

    Q_INVOKABLE void refresh();

signals:
    void profileChanged();

private:
    MangaDatabase *m_db = nullptr;

    QString m_displayName = QStringLiteral("Reader");
    QString m_username = QStringLiteral("@reader · Solo Hunter");
    QString m_bio = QStringLiteral("Passionate manga and web novel reader.");
    QString m_readingSince = QStringLiteral("October 2026");

    int m_chaptersRead = 0;
    int m_libraryCount = 0;
    int m_currentStreak = 0;

    QString m_rankName;
    QString m_rankTag;
    QString m_rankTier;
    QString m_rankCode;
    QString m_rankColor;
    QString m_rankBgColor;
    int m_score = 0;
    double m_progressToNextRank = 0.0;
    QString m_nextRankInfo;

    int m_unlockedMilestonesCount = 0;
    int m_totalMilestonesCount = 0;
    QVariantList m_unlockedBadges;
    QVariantList m_lockedBadges;
    QVariantList m_recentActivity;
    QVariantList m_pinnedCovers;
};

} // namespace mangadl
