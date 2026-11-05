/*
 * The PackageKit daemon: properties and global change notifications.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef TDEPK_PKDAEMON_H
#define TDEPK_PKDAEMON_H

#include <tqobject.h>
#include <tqstring.h>
#include <tqstringlist.h>

#include <tqdbusconnection.h>

#include "pkenums.h"

class TQT_DBusMessage;
class TQT_DBusProxy;

namespace PkDBus {
extern const char *const Service;
extern const char *const Path;
extern const char *const Interface;
extern const char *const TransactionInterface;
/* Ask the bus to route signals matching the rule to this connection. */
void addMatch(const TQT_DBusConnection &bus, const TQString &rule);
void removeMatch(const TQT_DBusConnection &bus, const TQString &rule);
}

class PkDaemon : public TQObject
{
    TQ_OBJECT
public:
    static PkDaemon *self();

    TQT_DBusConnection &bus() { return m_bus; }
    bool busConnected() const { return m_bus.isConnected(); }

    /* Known after propertiesChanged() has fired for the first time. */
    bool available() const { return m_available; }
    bool locked() const { return m_locked; }
    TQString backendName() const { return m_backendName; }
    TQString backendDescription() const { return m_backendDescription; }
    TQString distroId() const { return m_distroId; }
    TQString version() const { return m_version; }
    Pk::Bitfield roles() const { return m_roles; }
    Pk::Bitfield groups() const { return m_groups; }
    Pk::Bitfield filters() const { return m_filters; }
    uint networkState() const { return m_network; }

    bool canDo(Pk::Role role) const { return m_roles & Pk::bit(role); }

public slots:
    /* Read properties again. This also starts the daemon if it is not running. */
    void refreshProperties();
    /* Asks how many seconds ago a role last ran; answered by timeSinceAction(). */
    void queryTimeSinceAction(uint role);

signals:
    void propertiesChanged();
    void unavailable(const TQString &reason);
    void updatesChanged();
    void repoListChanged();
    void installedChanged();
    void transactionListChanged(const TQStringList &paths);
    void restartScheduled();
    /* The daemon's bus name lost its owner. It exits when idle, and restarts on demand. */
    void daemonLost();
    void daemonStarted();
    void timeSinceAction(uint role, uint seconds);

private slots:
    void onSignal(const TQT_DBusMessage &message);
    void onBusSignal(const TQT_DBusMessage &message);
    void onAsyncReply(int callId, const TQT_DBusMessage &message);

private:
    PkDaemon();
    void readProperties(const TQT_DBusMessage &reply);

    TQT_DBusConnection m_bus;
    TQT_DBusProxy *m_proxy;
    TQT_DBusProxy *m_propsProxy;
    TQT_DBusProxy *m_signalProxy;
    TQT_DBusProxy *m_busProxy;
    int m_propsCall;
    int m_timeCall;
    uint m_timeRole;

    bool m_available;
    bool m_locked;
    TQString m_backendName, m_backendDescription, m_distroId, m_version;
    Pk::Bitfield m_roles, m_groups, m_filters;
    uint m_network;
};

#endif
