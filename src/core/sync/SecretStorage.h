#pragma once

#include <QString>

namespace mangadl {

class SecretStorage {
public:
    static bool storeSecret(const QString &key, const QString &secret);
    static QString getSecret(const QString &key);
    static bool clearSecret(const QString &key);
    static bool isLibsecretAvailable();
};

} // namespace mangadl
