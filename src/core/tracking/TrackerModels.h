#pragma once

#include <QString>

namespace mangadl {

struct TrackerSearchResult {
    int id = 0;
    QString title;
    int totalChapters = 0;
    QString coverUrl;
    QString tracker; // "AniList" or "MyAnimeList"
};

struct TrackerTokenResult {
    QString accessToken;
    QString refreshToken;
    int expiresIn = 0;
    QString username;
    bool isValid() const { return !accessToken.isEmpty(); }
};

struct TrackerProfile {
    int id = 0;
    QString username;
    QString avatarUrl;
};

} // namespace mangadl
