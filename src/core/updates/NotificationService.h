#pragma once

#include <QObject>
#include <QString>

namespace mangadl {

class NotificationService : public QObject {
    Q_OBJECT

public:
    static NotificationService &instance();

    void notify(const QString &title, const QString &body, const QString &icon = QString());

private:
    explicit NotificationService(QObject *parent = nullptr);
    ~NotificationService() override = default;
};

} // namespace mangadl
