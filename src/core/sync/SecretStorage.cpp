#include "SecretStorage.h"
#include "src/core/settings/AppSettings.h"
#include <dlfcn.h>
#include <QByteArray>

namespace mangadl {

typedef struct _SecretSchema SecretSchema;
typedef struct _GCancellable GCancellable;
typedef struct _GError GError;

typedef int (*fn_secret_password_store_sync)(
    const SecretSchema *schema,
    const char *collection,
    const char *label,
    const char *password,
    GCancellable *cancellable,
    GError **error,
    ...
);

typedef char* (*fn_secret_password_lookup_sync)(
    const SecretSchema *schema,
    GCancellable *cancellable,
    GError **error,
    ...
);

typedef int (*fn_secret_password_clear_sync)(
    const SecretSchema *schema,
    GCancellable *cancellable,
    GError **error,
    ...
);

typedef void (*fn_secret_password_free)(char *password);

static void *getLibsecretHandle()
{
    static void *handle = dlopen("libsecret-1.so.0", RTLD_LAZY);
    return handle;
}

bool SecretStorage::isLibsecretAvailable()
{
    return getLibsecretHandle() != nullptr;
}

bool SecretStorage::storeSecret(const QString &key, const QString &secret)
{
    void *h = getLibsecretHandle();
    if (h) {
        auto fn_store = reinterpret_cast<fn_secret_password_store_sync>(dlsym(h, "secret_password_store_sync"));
        if (fn_store) {
            // Use NULL schema for simple key-value attributes
            const QByteArray k = key.toUtf8();
            const QByteArray v = secret.toUtf8();
            const char *label = "manga-dl credential";
            // secret_password_store_sync(NULL, SECRET_COLLECTION_DEFAULT, label, password, NULL, &error, "key", key, NULL);
            int res = fn_store(nullptr, nullptr, label, v.constData(), nullptr, nullptr, "key", k.constData(), nullptr);
            if (res) return true;
        }
    }

    // Fallback to AppSettings
    AppSettings::instance()->setAuthSecret(key, secret);
    return true;
}

QString SecretStorage::getSecret(const QString &key)
{
    void *h = getLibsecretHandle();
    if (h) {
        auto fn_lookup = reinterpret_cast<fn_secret_password_lookup_sync>(dlsym(h, "secret_password_lookup_sync"));
        auto fn_free = reinterpret_cast<fn_secret_password_free>(dlsym(h, "secret_password_free"));
        if (fn_lookup && fn_free) {
            const QByteArray k = key.toUtf8();
            char *pass = fn_lookup(nullptr, nullptr, nullptr, "key", k.constData(), nullptr);
            if (pass) {
                QString res = QString::fromUtf8(pass);
                fn_free(pass);
                return res;
            }
        }
    }

    // Fallback to AppSettings
    return AppSettings::instance()->authSecret(key);
}

bool SecretStorage::clearSecret(const QString &key)
{
    void *h = getLibsecretHandle();
    if (h) {
        auto fn_clear = reinterpret_cast<fn_secret_password_clear_sync>(dlsym(h, "secret_password_clear_sync"));
        if (fn_clear) {
            const QByteArray k = key.toUtf8();
            fn_clear(nullptr, nullptr, nullptr, "key", k.constData(), nullptr);
        }
    }

    AppSettings::instance()->removeAuthSecret(key);
    return true;
}

} // namespace mangadl
