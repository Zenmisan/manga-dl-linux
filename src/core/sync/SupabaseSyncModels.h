#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>

namespace mangadl {

struct SupabaseSubscriptionRecord {
    QString id;       // "{provider}/{mangaId}"
    QString userId;
    QString provider;
    QString mangaId;
    QString title;
    QString coverUrl;
    QString type;     // "manga" or "novel"
    QString addedAt;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj[QStringLiteral("id")] = id;
        obj[QStringLiteral("user_id")] = userId;
        obj[QStringLiteral("provider")] = provider;
        obj[QStringLiteral("manga_id")] = mangaId;
        obj[QStringLiteral("title")] = title;
        if (!coverUrl.isEmpty()) obj[QStringLiteral("cover_url")] = coverUrl;
        obj[QStringLiteral("type")] = type.isEmpty() ? QStringLiteral("manga") : type;
        if (!addedAt.isEmpty()) obj[QStringLiteral("added_at")] = addedAt;
        return obj;
    }

    static SupabaseSubscriptionRecord fromJson(const QJsonObject &obj) {
        SupabaseSubscriptionRecord rec;
        rec.id = obj.value(QStringLiteral("id")).toString();
        rec.userId = obj.value(QStringLiteral("user_id")).toString();
        rec.provider = obj.value(QStringLiteral("provider")).toString();
        rec.mangaId = obj.value(QStringLiteral("manga_id")).toString();
        rec.title = obj.value(QStringLiteral("title")).toString();
        rec.coverUrl = obj.value(QStringLiteral("cover_url")).toString();
        rec.type = obj.value(QStringLiteral("type")).toString(QStringLiteral("manga"));
        rec.addedAt = obj.value(QStringLiteral("added_at")).toString();
        return rec;
    }
};

struct SupabaseReadTrackingRecord {
    QString userId;
    QString provider;
    QString mangaId;
    QStringList chapterIds;
    QString updatedAt;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj[QStringLiteral("user_id")] = userId;
        obj[QStringLiteral("provider")] = provider;
        obj[QStringLiteral("manga_id")] = mangaId;
        QJsonArray arr;
        for (const auto &c : chapterIds) arr.append(c);
        obj[QStringLiteral("chapter_ids")] = arr;
        if (!updatedAt.isEmpty()) obj[QStringLiteral("updated_at")] = updatedAt;
        return obj;
    }

    static SupabaseReadTrackingRecord fromJson(const QJsonObject &obj) {
        SupabaseReadTrackingRecord rec;
        rec.userId = obj.value(QStringLiteral("user_id")).toString();
        rec.provider = obj.value(QStringLiteral("provider")).toString();
        rec.mangaId = obj.value(QStringLiteral("manga_id")).toString();
        const QJsonArray arr = obj.value(QStringLiteral("chapter_ids")).toArray();
        for (const auto &v : arr) rec.chapterIds.append(v.toString());
        rec.updatedAt = obj.value(QStringLiteral("updated_at")).toString();
        return rec;
    }
};

struct SupabaseReadingProgressRecord {
    QString userId;
    QString provider;
    QString mangaId;
    QString chapterId;
    int lastPage = 1;
    QString mangaTitle;
    QString chapterTitle;
    QString updatedAt;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj[QStringLiteral("user_id")] = userId;
        obj[QStringLiteral("provider")] = provider;
        obj[QStringLiteral("manga_id")] = mangaId;
        obj[QStringLiteral("chapter_id")] = chapterId;
        obj[QStringLiteral("last_page")] = lastPage;
        if (!mangaTitle.isEmpty()) obj[QStringLiteral("manga_title")] = mangaTitle;
        if (!chapterTitle.isEmpty()) obj[QStringLiteral("chapter_title")] = chapterTitle;
        if (!updatedAt.isEmpty()) obj[QStringLiteral("updated_at")] = updatedAt;
        return obj;
    }

    static SupabaseReadingProgressRecord fromJson(const QJsonObject &obj) {
        SupabaseReadingProgressRecord rec;
        rec.userId = obj.value(QStringLiteral("user_id")).toString();
        rec.provider = obj.value(QStringLiteral("provider")).toString();
        rec.mangaId = obj.value(QStringLiteral("manga_id")).toString();
        rec.chapterId = obj.value(QStringLiteral("chapter_id")).toString();
        rec.lastPage = obj.value(QStringLiteral("last_page")).toInt(1);
        rec.mangaTitle = obj.value(QStringLiteral("manga_title")).toString();
        rec.chapterTitle = obj.value(QStringLiteral("chapter_title")).toString();
        rec.updatedAt = obj.value(QStringLiteral("updated_at")).toString();
        return rec;
    }
};

struct SyncResult {
    bool success = false;
    int pulledLibraryCount = 0;
    int pulledProgressCount = 0;
    int pushedLibraryCount = 0;
    int pushedProgressCount = 0;
    QString errorMessage;
};

} // namespace mangadl
