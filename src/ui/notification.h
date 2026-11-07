// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// Copyright (c) 2026 madalone. Error pop-ups or the error banner, and the banners for unavailable devices
// (Settings → User interface → Messages).
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QUuid>

namespace uc {
namespace ui {

class NotificationItem : public QObject {
    Q_OBJECT

 public:
    explicit NotificationItem(const QString &id, QDateTime timestamp, const QString &title, const QString &message,
                              const QString &icon = QString(), void (*action)(QVariant) = nullptr,
                              QVariant param = QVariant(), const QString &actionLabel = QString(), bool warning = false,
                              QObject *parent = nullptr);
    ~NotificationItem();

    Q_INVOKABLE QString itemKey() const { return m_id; }
    QDateTime           itemTimestamp() const { return m_timestamp; }
    Q_INVOKABLE QString itemIcon() const { return m_icon; }
    Q_INVOKABLE QString itemTitle() const { return m_title; }
    Q_INVOKABLE QString itemMessage() const { return m_message; }
    Q_INVOKABLE QString itemActionLabel() const { return m_actionLabel; }
    Q_INVOKABLE bool    itemWarning() const { return m_warning; }

    /**
     * Two notifications are the same message when title and message match. The title alone is not
     * enough: "Error sending the command" is raised per entity and the entity is named in the
     * message only, so de-duplicating on the title hid every error but the first one.
     */
    Q_INVOKABLE bool isDuplicateOf(QObject *other) const;

    Q_INVOKABLE void action();

 private:
    QString   m_id;
    QDateTime m_timestamp;
    QString   m_icon;
    QString   m_title;
    QString   m_message;
    void      (*m_action)(QVariant);
    QVariant  m_param;
    QString   m_actionLabel;
    bool      m_warning;
};

//============================================================================================================================================//

class NotificationsModel : public QAbstractListModel {
    Q_OBJECT

 public:
    enum SearchRoles {
        KeyRole = Qt::UserRole + 1,
        TimestampRole,
        IconRole,
        TitleRole,
        MessageRole,
        ActionLabeRole,
        WarningRole
    };

    explicit NotificationsModel(QObject *parent = nullptr);
    ~NotificationsModel() = default;

 public:
    // Q_PROPERTY methods
    int count() const;

 public slots:
    // Q_PROPERTY methods
    void setCount(int count);

 public:
    // QQAbstractListModel overrides and required emthods
    int                    rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool                   removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    QVariant               data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

 public:
    // Custom methods
    void append(NotificationItem *o);
    void clear();

    QModelIndex getModelIndexByKey(const QString &key);

    NotificationItem *get(const QString &key);
    NotificationItem *get(int row);

    void removeItem(const QString &key);
    void removeItem(int row);

 signals:
    void countChanged(int count);

 private:
    int                       m_count;
    QList<NotificationItem *> m_data;
};

//============================================================================================================================================//

class Notification : public QObject {
    Q_OBJECT

    Q_PROPERTY(QAbstractListModel *model READ getModel CONSTANT)

 public:
    explicit Notification(QObject *parent = nullptr);
    ~Notification();

    QAbstractListModel *getModel() { return &m_notifications; }

    Q_INVOKABLE void remove(const QString &id);
    Q_INVOKABLE void clearAll();

    // madalone (v1.6.3): the QML callers' entry point (the touch sliders), so the rule lives here only: false while
    // the error pop-ups are on, and the caller opens its pop-up; otherwise the error banner (createErrorBanner) and
    // true
    Q_INVOKABLE bool bannerInsteadOfPopUp(const QString &message, bool warning);
    // madalone (v1.6.3): the QML callers of createUnavailableBanner() (the tiles)
    Q_INVOKABLE void showUnavailableBanner(const QString &message, bool warning) {
        createUnavailableBanner(message, warning);
    }

 public:
    static void createNotification(const QString &message, bool warning = false);
    static void createActionableNotification(const QString &title, const QString &message,
                                             const QString &icon = QString(), void (*action)(QVariant) = nullptr,
                                             QVariant param = QVariant(), const QString &actionLabel = QString());
    static void createActionableWarningNotification(const QString &title, const QString &message,
                                                    const QString &icon = QString(), void (*action)(QVariant) = nullptr,
                                                    QVariant       param = QVariant(),
                                                    const QString &actionLabel = QString());

    /**
     * madalone (v1.6.3): Settings → User interface → Messages. Whether failed commands, the touch slider's
     * "not available" and integration errors open the pop-up that takes the input until it is dismissed (popups,
     * upstream's behaviour), and whether, with the pop-ups off, they show the error banner (banner). main.cpp pushes
     * both switches from Config at start-up and on every change.
     */
    static void setErrorFeedback(bool popups, bool banner);
    // true while the error pop-ups are on, and before setErrorFeedback() or without an instance
    static bool errorPopupsEnabled();
    // a banner in place of an error pop-up: shown with the error banner duration while "Show a banner" is
    // on, nothing otherwise; like every banner it never takes the input
    static void createErrorBanner(const QString &message, bool warning);
    /**
     * madalone (v1.6.3): Settings → User interface → Messages → "Show offline banners", pushed by
     * main.cpp like the error switches. The "%1 is unavailable" banner of a tile or button pressed for an unavailable
     * device goes through createUnavailableBanner(): an ordinary banner (createNotification) while the switch is on,
     * nothing otherwise. Every other banner keeps createNotification() and always shows.
     */
    static void setUnavailableBanners(bool show);
    static void createUnavailableBanner(const QString &message, bool warning);

    static QObject *qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine);

 signals:
    void notificationCreated(QString message, bool warning);
    void actionableNotificationCreated(QObject *notification);
    // madalone (v1.6.3)
    void errorBannerCreated(QString message, bool warning);

 private:
    static Notification *s_instance;

    NotificationsModel m_notifications;

    // madalone (v1.6.3): stock behaviour until main.cpp pushes the stored settings
    bool m_errorPopups = true;
    bool m_errorBanner = true;
    bool m_unavailableBanners = true;
};
}  // namespace ui
}  // namespace uc
