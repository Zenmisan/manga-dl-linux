#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>
#include <QJSEngine>

namespace mangadl {

class MangaDatabase;

class StatsController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int chaptersRead READ chaptersRead NOTIFY statsChanged)
    Q_PROPERTY(QString timeReading READ timeReading NOTIFY statsChanged)
    Q_PROPERTY(int currentStreak READ currentStreak NOTIFY statsChanged)
    Q_PROPERTY(QString readingPace READ readingPace NOTIFY statsChanged)

    Q_PROPERTY(double monthlyGoalProgress READ monthlyGoalProgress NOTIFY statsChanged)
    Q_PROPERTY(QString monthlyGoalText READ monthlyGoalText NOTIFY statsChanged)
    Q_PROPERTY(double yearlyGoalProgress READ yearlyGoalProgress NOTIFY statsChanged)
    Q_PROPERTY(QString yearlyGoalText READ yearlyGoalText NOTIFY statsChanged)

    Q_PROPERTY(QVariantList activityHeatmap READ activityHeatmap NOTIFY statsChanged)
    Q_PROPERTY(QVariantList byCategory READ byCategory NOTIFY statsChanged)
    Q_PROPERTY(QVariantList bySource READ bySource NOTIFY statsChanged)

    Q_PROPERTY(QString rankName READ rankName NOTIFY statsChanged)
    Q_PROPERTY(QString rankCode READ rankCode NOTIFY statsChanged)
    Q_PROPERTY(QString rankTier READ rankTier NOTIFY statsChanged)
    Q_PROPERTY(QString rankTag READ rankTag NOTIFY statsChanged)
    Q_PROPERTY(QString rankColor READ rankColor NOTIFY statsChanged)
    Q_PROPERTY(QString rankBgColor READ rankBgColor NOTIFY statsChanged)
    Q_PROPERTY(int score READ score NOTIFY statsChanged)
    Q_PROPERTY(double progressToNextRank READ progressToNextRank NOTIFY statsChanged)

public:
    explicit StatsController(MangaDatabase *db = nullptr, QObject *parent = nullptr);
    ~StatsController() override = default;

    static StatsController &instance();
    static StatsController *create(QQmlEngine *engine, QJSEngine *scriptEngine);

    int chaptersRead() const { return m_chaptersRead; }
    QString timeReading() const { return m_timeReading; }
    int currentStreak() const { return m_currentStreak; }
    QString readingPace() const { return m_readingPace; }

    double monthlyGoalProgress() const { return m_monthlyGoalProgress; }
    QString monthlyGoalText() const { return m_monthlyGoalText; }
    double yearlyGoalProgress() const { return m_yearlyGoalProgress; }
    QString yearlyGoalText() const { return m_yearlyGoalText; }

    QVariantList activityHeatmap() const { return m_activityHeatmap; }
    QVariantList byCategory() const { return m_byCategory; }
    QVariantList bySource() const { return m_bySource; }

    QString rankName() const { return m_rankName; }
    QString rankCode() const { return m_rankCode; }
    QString rankTier() const { return m_rankTier; }
    QString rankTag() const { return m_rankTag; }
    QString rankColor() const { return m_rankColor; }
    QString rankBgColor() const { return m_rankBgColor; }
    int score() const { return m_score; }
    double progressToNextRank() const { return m_progressToNextRank; }

    Q_INVOKABLE void refresh();

signals:
    void statsChanged();

private:
    MangaDatabase *m_db = nullptr;

    int m_chaptersRead = 0;
    QString m_timeReading = QStringLiteral("0h");
    int m_currentStreak = 0;
    QString m_readingPace = QStringLiteral("0/wk");

    double m_monthlyGoalProgress = 0.0;
    QString m_monthlyGoalText = QStringLiteral("0 / 25");
    double m_yearlyGoalProgress = 0.0;
    QString m_yearlyGoalText = QStringLiteral("0 / 50");

    QVariantList m_activityHeatmap;
    QVariantList m_byCategory;
    QVariantList m_bySource;

    QString m_rankName;
    QString m_rankCode;
    QString m_rankTier;
    QString m_rankTag;
    QString m_rankColor;
    QString m_rankBgColor;
    int m_score = 0;
    double m_progressToNextRank = 0.0;
};

} // namespace mangadl
