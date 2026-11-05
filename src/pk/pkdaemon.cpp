/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "pkdaemon.h"

#include <tqdbusdata.h>
#include <tqdbusdatalist.h>
#include <tqdbusdatamap.h>
#include <tqdbuserror.h>
#include <tqdbusmessage.h>
#include <tqdbusproxy.h>
#include <tqdbusvariant.h>
#include <tqvaluelist.h>

#include <tdelocale.h>

namespace PkDBus {

const char *const Service = "org.freedesktop.PackageKit";
const char *const Path = "/org/freedesktop/PackageKit";
const char *const Interface = "org.freedesktop.PackageKit";
const char *const TransactionInterface = "org.freedesktop.PackageKit.Transaction";

static void callBusMatch(const TQT_DBusConnection &bus, const char *method, const TQString &rule)
{
    TQT_DBusMessage message = TQT_DBusMessage::methodCall(
        "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", method);
    message << TQT_DBusData::fromString(rule);
    bus.send(message);
}

void addMatch(const TQT_DBusConnection &bus, const TQString &rule)
{
    callBusMatch(bus, "AddMatch", rule);
}

void removeMatch(const TQT_DBusConnection &bus, const TQString &rule)
{
    callBusMatch(bus, "RemoveMatch", rule);
}

} // namespace PkDBus

static TQ_UINT64 variantToUInt64(const TQT_DBusData &data)
{
    TQT_DBusData value = data;
    if (value.type() == TQT_DBusData::Variant) {
        value = value.toVariant().value;
    }
    switch (value.type()) {
    case TQT_DBusData::UInt64: return value.toUInt64();
    case TQT_DBusData::Int64:  return value.toInt64();
    case TQT_DBusData::UInt32: return value.toUInt32();
    case TQT_DBusData::Int32:  return value.toInt32();
    default:                   return 0;
    }
}

static TQString variantToString(const TQT_DBusData &data)
{
    TQT_DBusData value = data;
    if (value.type() == TQT_DBusData::Variant) {
        value = value.toVariant().value;
    }
    return value.type() == TQT_DBusData::String ? value.toString() : TQString();
}

static bool variantToBool(const TQT_DBusData &data)
{
    TQT_DBusData value = data;
    if (value.type() == TQT_DBusData::Variant) {
        value = value.toVariant().value;
    }
    return value.type() == TQT_DBusData::Bool ? value.toBool() : false;
}

PkDaemon *PkDaemon::self()
{
    static PkDaemon *instance = nullptr;
    if (!instance) {
        instance = new PkDaemon();
    }
    return instance;
}

PkDaemon::PkDaemon()
    : TQObject(nullptr, "pkdaemon"),
      m_bus(TQT_DBusConnection::systemBus()),
      m_propsCall(-1), m_timeCall(-1), m_timeRole(0),
      m_available(false), m_locked(false),
      m_roles(0), m_groups(0), m_filters(0), m_network(0)
{
    m_proxy = new TQT_DBusProxy(PkDBus::Service, PkDBus::Path, PkDBus::Interface, m_bus, this);
    m_propsProxy = new TQT_DBusProxy(PkDBus::Service, PkDBus::Path,
                                     "org.freedesktop.DBus.Properties", m_bus, this);
    /* An empty interface also lets PropertiesChanged through. */
    m_signalProxy = new TQT_DBusProxy(TQString(), PkDBus::Path, TQString(), m_bus, this);
    m_busProxy = new TQT_DBusProxy(TQString(), "/org/freedesktop/DBus",
                                   "org.freedesktop.DBus", m_bus, this);

    connect(m_signalProxy, TQ_SIGNAL(dbusSignal(const TQT_DBusMessage &)),
            this, TQ_SLOT(onSignal(const TQT_DBusMessage &)));
    connect(m_busProxy, TQ_SIGNAL(dbusSignal(const TQT_DBusMessage &)),
            this, TQ_SLOT(onBusSignal(const TQT_DBusMessage &)));
    connect(m_propsProxy, TQ_SIGNAL(asyncReply(int, const TQT_DBusMessage &)),
            this, TQ_SLOT(onAsyncReply(int, const TQT_DBusMessage &)));
    connect(m_proxy, TQ_SIGNAL(asyncReply(int, const TQT_DBusMessage &)),
            this, TQ_SLOT(onAsyncReply(int, const TQT_DBusMessage &)));

    if (m_bus.isConnected()) {
        PkDBus::addMatch(m_bus, TQString("type='signal',path='%1'").arg(PkDBus::Path));
        PkDBus::addMatch(m_bus, TQString("type='signal',sender='org.freedesktop.DBus',"
                                         "member='NameOwnerChanged',arg0='%1'").arg(PkDBus::Service));
    }
}

void PkDaemon::refreshProperties()
{
    if (!m_bus.isConnected()) {
        emit unavailable(i18n("Could not connect to the system message bus."));
        return;
    }
    TQValueList<TQT_DBusData> params;
    params << TQT_DBusData::fromString(PkDBus::Interface);
    m_propsCall = m_propsProxy->sendWithAsyncReply("GetAll", params);
}

void PkDaemon::queryTimeSinceAction(uint role)
{
    TQValueList<TQT_DBusData> params;
    params << TQT_DBusData::fromUInt32(role);
    m_timeRole = role;
    m_timeCall = m_proxy->sendWithAsyncReply("GetTimeSinceAction", params);
}

void PkDaemon::onAsyncReply(int callId, const TQT_DBusMessage &message)
{
    if (callId == m_propsCall) {
        m_propsCall = -1;
        if (message.type() == TQT_DBusMessage::ReplyMessage) {
            readProperties(message);
        } else {
            m_available = false;
            emit unavailable(message.error().message());
        }
    } else if (callId == m_timeCall) {
        m_timeCall = -1;
        const uint seconds = (message.type() == TQT_DBusMessage::ReplyMessage && message.count() == 1)
                                 ? message[0].toUInt32()
                                 : TQ_UINT32(-1);
        emit timeSinceAction(m_timeRole, seconds);
    }
}

void PkDaemon::readProperties(const TQT_DBusMessage &reply)
{
    if (reply.count() != 1 || reply[0].type() != TQT_DBusData::Map) {
        emit unavailable(i18n("The package system sent an unexpected reply."));
        return;
    }
    const TQT_DBusDataMap<TQString> map = reply[0].toStringKeyMap();
    uint major = 0, minor = 0, micro = 0;
    for (TQT_DBusDataMap<TQString>::const_iterator it = map.begin(); it != map.end(); ++it) {
        const TQString &key = it.key();
        const TQT_DBusData &value = it.data();
        if (key == "BackendName") m_backendName = variantToString(value);
        else if (key == "BackendDescription") m_backendDescription = variantToString(value);
        else if (key == "DistroId") m_distroId = variantToString(value);
        else if (key == "Roles") m_roles = variantToUInt64(value);
        else if (key == "Groups") m_groups = variantToUInt64(value);
        else if (key == "Filters") m_filters = variantToUInt64(value);
        else if (key == "Locked") m_locked = variantToBool(value);
        else if (key == "NetworkState") m_network = uint(variantToUInt64(value));
        else if (key == "VersionMajor") major = uint(variantToUInt64(value));
        else if (key == "VersionMinor") minor = uint(variantToUInt64(value));
        else if (key == "VersionMicro") micro = uint(variantToUInt64(value));
    }
    m_version = TQString("%1.%2.%3").arg(major).arg(minor).arg(micro);
    m_available = true;
    emit propertiesChanged();
}

void PkDaemon::onSignal(const TQT_DBusMessage &message)
{
    const TQString member = message.member();
    if (member == "UpdatesChanged") {
        emit updatesChanged();
    } else if (member == "RepoListChanged") {
        emit repoListChanged();
    } else if (member == "InstalledChanged") {
        emit installedChanged();
    } else if (member == "RestartSchedule") {
        emit restartScheduled();
    } else if (member == "TransactionListChanged") {
        TQStringList paths;
        if (message.count() == 1 && message[0].type() == TQT_DBusData::List) {
            paths = message[0].toList().toTQStringList();
        }
        emit transactionListChanged(paths);
    } else if (member == "PropertiesChanged") {
        refreshProperties();
    }
}

void PkDaemon::onBusSignal(const TQT_DBusMessage &message)
{
    if (message.member() != "NameOwnerChanged" || message.count() != 3 ||
        message[0].toString() != PkDBus::Service) {
        return;
    }
    if (message[2].toString().isEmpty()) {
        m_available = false;
        emit daemonLost();
    } else {
        emit daemonStarted();
        refreshProperties();
    }
}

#include "pkdaemon.moc"
