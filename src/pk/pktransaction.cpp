/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "pktransaction.h"

#include <stdlib.h>

#include <tqdbusdata.h>
#include <tqdbusdatalist.h>
#include <tqdbusdatamap.h>
#include <tqdbuserror.h>
#include <tqdbusmessage.h>
#include <tqdbusobjectpath.h>
#include <tqdbusproxy.h>
#include <tqdbusvariant.h>

#include <tdeglobal.h>
#include <tdelocale.h>

#include "pkdaemon.h"

/* PackageKit's "unknown" percentage. */
static const uint PercentageUnknown = 101;

static TQT_DBusData stringListData(const TQStringList &list)
{
    /* An empty list needs its element type stated for the signature "as". */
    TQT_DBusDataList dataList(TQT_DBusData::String);
    if (!list.isEmpty()) {
        dataList = TQT_DBusDataList(list);
    }
    return TQT_DBusData::fromList(dataList);
}

static TQStringList toStringList(const TQT_DBusData &data)
{
    return data.type() == TQT_DBusData::List ? data.toList().toTQStringList() : TQStringList();
}

static TQString objectPathString(const TQT_DBusData &data)
{
    return data.toObjectPath();
}

static TQT_DBusData unwrap(const TQT_DBusData &data)
{
    return data.type() == TQT_DBusData::Variant ? data.toVariant().value : data;
}

static TQ_UINT64 toUInt64(const TQT_DBusData &data)
{
    const TQT_DBusData value = unwrap(data);
    switch (value.type()) {
    case TQT_DBusData::UInt64: return value.toUInt64();
    case TQT_DBusData::Int64:  return value.toInt64();
    case TQT_DBusData::UInt32: return value.toUInt32();
    case TQT_DBusData::Int32:  return value.toInt32();
    default:                   return 0;
    }
}

static TQString toStringValue(const TQT_DBusData &data)
{
    const TQT_DBusData value = unwrap(data);
    return value.type() == TQT_DBusData::String ? value.toString() : TQString();
}

static PkUpdateDetail updateDetailFrom(const TQValueList<TQT_DBusData> &f)
{
    PkUpdateDetail d;
    if (f.count() == 12) {
        d.id = f[0].toString();
        d.updates = toStringList(f[1]);
        d.obsoletes = toStringList(f[2]);
        d.vendorUrls = toStringList(f[3]);
        d.bugzillaUrls = toStringList(f[4]);
        d.cveUrls = toStringList(f[5]);
        d.restart = f[6].toUInt32();
        d.updateText = f[7].toString();
        d.changelog = f[8].toString();
        d.state = f[9].toUInt32();
        d.issued = f[10].toString();
        d.updated = f[11].toString();
    }
    return d;
}

PkTransaction::PkTransaction(TQObject *parent, const char *name)
    : TQObject(parent, name),
      m_daemonProxy(nullptr), m_proxy(nullptr), m_signalProxy(nullptr),
      m_createCall(-1), m_methodCall(-1),
      m_status(Pk::StatusUnknown), m_percentage(PercentageUnknown),
      m_allowCancel(false), m_cancelPending(false), m_finished(false), m_started(false)
{
    connect(PkDaemon::self(), TQ_SIGNAL(daemonLost()), this, TQ_SLOT(onDaemonLost()));
}

PkTransaction::~PkTransaction()
{
    releaseMatch();
}

void PkTransaction::releaseMatch()
{
    if (!m_match.isEmpty()) {
        PkDBus::removeMatch(PkDaemon::self()->bus(), m_match);
        m_match = TQString();
    }
}

void PkTransaction::start(const TQString &method, const TQValueList<TQT_DBusData> &params)
{
    if (m_started) {
        tqWarning("PkTransaction: %s called on a used transaction", method.latin1());
        return;
    }
    m_started = true;
    m_method = method;
    m_params = params;

    PkDaemon *daemon = PkDaemon::self();
    if (!daemon->busConnected()) {
        fail(Pk::ErrorInternalError, i18n("Could not connect to the system message bus."));
        return;
    }
    m_daemonProxy = new TQT_DBusProxy(PkDBus::Service, PkDBus::Path, PkDBus::Interface,
                                      daemon->bus(), this);
    connect(m_daemonProxy, TQ_SIGNAL(asyncReply(int, const TQT_DBusMessage &)),
            this, TQ_SLOT(onDaemonReply(int, const TQT_DBusMessage &)));
    m_createCall = m_daemonProxy->sendWithAsyncReply("CreateTransaction", TQValueList<TQT_DBusData>());
    if (m_createCall < 0) {
        fail(Pk::ErrorInternalError, i18n("Could not contact the package system."));
    }
}

void PkTransaction::onDaemonReply(int callId, const TQT_DBusMessage &message)
{
    if (callId != m_createCall || m_finished) {
        return;
    }
    m_createCall = -1;
    if (message.type() != TQT_DBusMessage::ReplyMessage || message.count() != 1) {
        fail(Pk::ErrorInternalError,
             i18n("The PackageKit service is not available: %1").arg(message.error().message()));
        return;
    }
    if (m_cancelPending) {
        finish(Pk::ExitCancelled, 0);
        return;
    }

    PkDaemon *daemon = PkDaemon::self();
    m_path = objectPathString(message[0]);
    m_match = TQString("type='signal',path='%1'").arg(m_path);
    PkDBus::addMatch(daemon->bus(), m_match);

    m_signalProxy = new TQT_DBusProxy(TQString(), m_path, TQString(), daemon->bus(), this);
    connect(m_signalProxy, TQ_SIGNAL(dbusSignal(const TQT_DBusMessage &)),
            this, TQ_SLOT(onSignal(const TQT_DBusMessage &)));

    m_proxy = new TQT_DBusProxy(PkDBus::Service, m_path, PkDBus::TransactionInterface,
                                daemon->bus(), this);
    connect(m_proxy, TQ_SIGNAL(asyncReply(int, const TQT_DBusMessage &)),
            this, TQ_SLOT(onMethodReply(int, const TQT_DBusMessage &)));

    TQStringList hints;
    const TQString lang = TDEGlobal::locale()->language();
    const char *envLang = getenv("LANG");
    hints << TQString("locale=%1").arg(envLang && *envLang ? TQString::fromLatin1(envLang) : lang);
    hints << "interactive=true";
    TQValueList<TQT_DBusData> hintParams;
    hintParams << stringListData(hints);
    m_proxy->send("SetHints", hintParams);

    m_methodCall = m_proxy->sendWithAsyncReply(m_method, m_params);
    if (m_methodCall < 0) {
        fail(Pk::ErrorInternalError, i18n("Could not contact the package system."));
    }
}

void PkTransaction::onMethodReply(int callId, const TQT_DBusMessage &message)
{
    if (callId != m_methodCall || m_finished) {
        return;
    }
    m_methodCall = -1;
    if (message.type() == TQT_DBusMessage::ErrorMessage) {
        const TQString name = message.error().name();
        const uint code = name.contains("Refused") || name.contains("AccessDenied")
                              ? uint(Pk::ErrorNotAuthorized)
                              : uint(Pk::ErrorInternalError);
        fail(code, message.error().message());
    }
}

void PkTransaction::fail(uint code, const TQString &details)
{
    emit errorCode(code, details);
    finish(Pk::ExitFailed, 0);
}

void PkTransaction::finish(uint exit, uint runtime)
{
    if (m_finished) {
        return;
    }
    m_finished = true;
    releaseMatch();
    emit finished(exit, runtime);
}

void PkTransaction::onDaemonLost()
{
    if (m_started && !m_finished) {
        fail(Pk::ErrorInternalError, i18n("The PackageKit service stopped unexpectedly."));
    }
}

void PkTransaction::cancel()
{
    if (m_finished) {
        return;
    }
    if (!m_proxy) {
        m_cancelPending = true;
        if (!m_started) {
            m_started = true;
            finish(Pk::ExitCancelled, 0);
        }
        return;
    }
    m_proxy->send("Cancel", TQValueList<TQT_DBusData>());
}

void PkTransaction::applyProperty(const TQString &key, const TQT_DBusData &value)
{
    if (key == "Status") {
        const uint status = uint(toUInt64(value));
        if (status != m_status) {
            m_status = status;
            emit statusChanged(status);
        }
    } else if (key == "Percentage") {
        const uint percentage = uint(toUInt64(value));
        if (percentage != m_percentage) {
            m_percentage = percentage;
            emit progressChanged(percentage);
        }
    } else if (key == "AllowCancel") {
        const TQT_DBusData inner = unwrap(value);
        const bool allowed = inner.type() == TQT_DBusData::Bool && inner.toBool();
        if (allowed != m_allowCancel) {
            m_allowCancel = allowed;
            emit allowCancelChanged(allowed);
        }
    }
}

void PkTransaction::readChanged(const TQT_DBusMessage &message)
{
    if (message.count() < 2 || message[0].toString() != PkDBus::TransactionInterface ||
        message[1].type() != TQT_DBusData::Map) {
        return;
    }
    const TQT_DBusDataMap<TQString> map = message[1].toStringKeyMap();
    for (TQT_DBusDataMap<TQString>::const_iterator it = map.begin(); it != map.end(); ++it) {
        applyProperty(it.key(), it.data());
    }
}

void PkTransaction::onSignal(const TQT_DBusMessage &m)
{
    if (m_finished) {
        return;
    }
    const TQString member = m.member();
    if (member == "PropertiesChanged") {
        readChanged(m);
    } else if (member == "Package" && m.count() == 3) {
        emit package(PkPackage(m[0].toUInt32(), m[1].toString(), m[2].toString()));
    } else if (member == "Packages" && m.count() == 1 && m[0].type() == TQT_DBusData::List) {
        const TQValueList<TQT_DBusData> items = m[0].toList().toTQValueList();
        for (TQValueList<TQT_DBusData>::const_iterator it = items.begin(); it != items.end(); ++it) {
            const TQValueList<TQT_DBusData> f = (*it).toStruct();
            if (f.count() == 3) {
                emit package(PkPackage(f[0].toUInt32(), f[1].toString(), f[2].toString()));
            }
        }
    } else if (member == "Details" && m.count() == 1 && m[0].type() == TQT_DBusData::Map) {
        const TQT_DBusDataMap<TQString> map = m[0].toStringKeyMap();
        PkDetails d;
        for (TQT_DBusDataMap<TQString>::const_iterator it = map.begin(); it != map.end(); ++it) {
            const TQString &key = it.key();
            if (key == "package-id") d.id = toStringValue(it.data());
            else if (key == "group") d.group = uint(toUInt64(it.data()));
            else if (key == "summary") d.summary = toStringValue(it.data());
            else if (key == "description") d.description = toStringValue(it.data());
            else if (key == "url") d.url = toStringValue(it.data());
            else if (key == "license") d.license = toStringValue(it.data());
            else if (key == "size") d.size = toUInt64(it.data());
        }
        emit details(d);
    } else if (member == "Files" && m.count() == 2) {
        emit files(m[0].toString(), toStringList(m[1]));
    } else if (member == "UpdateDetail" && m.count() == 12) {
        TQValueList<TQT_DBusData> fields;
        for (uint i = 0; i < m.count(); ++i) {
            fields << m[i];
        }
        emit updateDetail(updateDetailFrom(fields));
    } else if (member == "UpdateDetails" && m.count() == 1 && m[0].type() == TQT_DBusData::List) {
        const TQValueList<TQT_DBusData> items = m[0].toList().toTQValueList();
        for (TQValueList<TQT_DBusData>::const_iterator it = items.begin(); it != items.end(); ++it) {
            emit updateDetail(updateDetailFrom((*it).toStruct()));
        }
    } else if (member == "RepoDetail" && m.count() == 3) {
        PkRepo repo;
        repo.id = m[0].toString();
        repo.description = m[1].toString();
        repo.enabled = m[2].toBool();
        emit repoDetail(repo);
    } else if (member == "Transaction" && m.count() == 8) {
        PkHistoryEntry e;
        e.path = objectPathString(m[0]);
        e.timespec = m[1].toString();
        e.succeeded = m[2].toBool();
        e.role = m[3].toUInt32();
        e.duration = m[4].toUInt32();
        e.data = m[5].toString();
        e.uid = m[6].toUInt32();
        e.cmdline = m[7].toString();
        emit historyEntry(e);
    } else if (member == "ItemProgress" && m.count() == 3) {
        emit itemProgress(m[0].toString(), m[1].toUInt32(), m[2].toUInt32());
    } else if (member == "ErrorCode" && m.count() == 2) {
        emit errorCode(m[0].toUInt32(), m[1].toString());
    } else if (member == "RepoSignatureRequired" && m.count() == 8) {
        PkSignature s;
        s.packageId = m[0].toString();
        s.repository = m[1].toString();
        s.keyUrl = m[2].toString();
        s.keyUserId = m[3].toString();
        s.keyId = m[4].toString();
        s.keyFingerprint = m[5].toString();
        s.keyTimestamp = m[6].toString();
        s.type = m[7].toUInt32();
        emit signatureRequired(s);
    } else if (member == "EulaRequired" && m.count() == 4) {
        PkEula e;
        e.id = m[0].toString();
        e.packageId = m[1].toString();
        e.vendor = m[2].toString();
        e.agreement = m[3].toString();
        emit eulaRequired(e);
    } else if (member == "RequireRestart" && m.count() == 2) {
        emit restartRequired(m[0].toUInt32(), m[1].toString());
    } else if (member == "Finished" && m.count() == 2) {
        finish(m[0].toUInt32(), m[1].toUInt32());
    } else if (member == "Destroy") {
        if (!m_finished) {
            fail(Pk::ErrorInternalError, i18n("The operation ended without a result."));
        }
    }
}

/* ------------------------------------------------------------------------ */

static TQValueList<TQT_DBusData> filterAndList(Pk::Bitfield filters, const TQStringList &list)
{
    TQValueList<TQT_DBusData> p;
    p << TQT_DBusData::fromUInt64(filters) << stringListData(list);
    return p;
}

static TQValueList<TQT_DBusData> oneList(const TQStringList &list)
{
    TQValueList<TQT_DBusData> p;
    p << stringListData(list);
    return p;
}

static TQValueList<TQT_DBusData> oneFilter(Pk::Bitfield filters)
{
    TQValueList<TQT_DBusData> p;
    p << TQT_DBusData::fromUInt64(filters);
    return p;
}

void PkTransaction::searchNames(Pk::Bitfield f, const TQStringList &t) { start("SearchNames", filterAndList(f, t)); }
void PkTransaction::searchDetails(Pk::Bitfield f, const TQStringList &t) { start("SearchDetails", filterAndList(f, t)); }
void PkTransaction::searchFiles(Pk::Bitfield f, const TQStringList &t) { start("SearchFiles", filterAndList(f, t)); }
void PkTransaction::searchGroups(Pk::Bitfield f, const TQStringList &g) { start("SearchGroups", filterAndList(f, g)); }
void PkTransaction::resolve(Pk::Bitfield f, const TQStringList &n) { start("Resolve", filterAndList(f, n)); }
void PkTransaction::getPackages(Pk::Bitfield f) { start("GetPackages", oneFilter(f)); }
void PkTransaction::getDetails(const TQStringList &ids) { start("GetDetails", oneList(ids)); }
void PkTransaction::getFiles(const TQStringList &ids) { start("GetFiles", oneList(ids)); }
void PkTransaction::getUpdates(Pk::Bitfield f) { start("GetUpdates", oneFilter(f)); }
void PkTransaction::getUpdateDetail(const TQStringList &ids) { start("GetUpdateDetail", oneList(ids)); }
void PkTransaction::getRepoList(Pk::Bitfield f) { start("GetRepoList", oneFilter(f)); }
void PkTransaction::installPackages(Pk::Bitfield f, const TQStringList &ids) { start("InstallPackages", filterAndList(f, ids)); }
void PkTransaction::updatePackages(Pk::Bitfield f, const TQStringList &ids) { start("UpdatePackages", filterAndList(f, ids)); }
void PkTransaction::installFiles(Pk::Bitfield f, const TQStringList &paths) { start("InstallFiles", filterAndList(f, paths)); }

void PkTransaction::dependsOn(Pk::Bitfield f, const TQStringList &ids, bool recursive)
{
    TQValueList<TQT_DBusData> p = filterAndList(f, ids);
    p << TQT_DBusData::fromBool(recursive);
    start("DependsOn", p);
}

void PkTransaction::requiredBy(Pk::Bitfield f, const TQStringList &ids, bool recursive)
{
    TQValueList<TQT_DBusData> p = filterAndList(f, ids);
    p << TQT_DBusData::fromBool(recursive);
    start("RequiredBy", p);
}

void PkTransaction::repoEnable(const TQString &repoId, bool enabled)
{
    TQValueList<TQT_DBusData> p;
    p << TQT_DBusData::fromString(repoId) << TQT_DBusData::fromBool(enabled);
    start("RepoEnable", p);
}

void PkTransaction::getOldTransactions(uint count)
{
    TQValueList<TQT_DBusData> p;
    p << TQT_DBusData::fromUInt32(count);
    start("GetOldTransactions", p);
}

void PkTransaction::refreshCache(bool force)
{
    TQValueList<TQT_DBusData> p;
    p << TQT_DBusData::fromBool(force);
    start("RefreshCache", p);
}

void PkTransaction::removePackages(Pk::Bitfield f, const TQStringList &ids, bool allowDeps, bool autoremove)
{
    TQValueList<TQT_DBusData> p = filterAndList(f, ids);
    p << TQT_DBusData::fromBool(allowDeps) << TQT_DBusData::fromBool(autoremove);
    start("RemovePackages", p);
}

void PkTransaction::installSignature(uint type, const TQString &keyId, const TQString &packageId)
{
    TQValueList<TQT_DBusData> p;
    p << TQT_DBusData::fromUInt32(type) << TQT_DBusData::fromString(keyId) << TQT_DBusData::fromString(packageId);
    start("InstallSignature", p);
}

void PkTransaction::acceptEula(const TQString &eulaId)
{
    TQValueList<TQT_DBusData> p;
    p << TQT_DBusData::fromString(eulaId);
    start("AcceptEula", p);
}

#include "pktransaction.moc"
