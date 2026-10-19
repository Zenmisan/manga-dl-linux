#pragma once

#include <QString>
#include <QJsonObject>
#include <optional>

namespace mangadl {

struct AuthSession {
    QString accessToken;
    QString refreshToken;
    QString userId;
    QString email;
    QString displayName;

    bool isValid() const {
        return !accessToken.isEmpty() && !userId.isEmpty();
    }
};

struct AuthSignUpResult {
    std::optional<AuthSession> session;
    QString message;
};

} // namespace mangadl
