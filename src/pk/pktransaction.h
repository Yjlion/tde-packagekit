/*
 * One PackageKit transaction.
 *
 * Call exactly one action method. Everything is asynchronous: results arrive
 * through signals, and finished() is always emitted exactly once, whether the
 * action ran, failed to start, or the daemon went away. The object can be
 * deleted from a slot connected to finished() with deleteLater().
 *
 * Slots connected to these signals, and to PkDaemon's, must not run a nested
 * event loop (a modal dialog, KMessageBox, ...). libdbus-1-tqt holds a list
 * iterator across each emission; a nested loop re-enters that code and frees the
 * node, and the application crashes when the dialog closes. It would also run
 * any pending deleteLater() while this object is still emitting. Leave the
 * callback first, with TQTimer::singleShot(0, ...).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef TDEPK_PKTRANSACTION_H
#define TDEPK_PKTRANSACTION_H

#include <tqobject.h>
#include <tqstring.h>
#include <tqstringlist.h>
#include <tqvaluelist.h>

#include "pkenums.h"
#include "pktypes.h"

class TQT_DBusData;
class TQT_DBusMessage;
class TQT_DBusProxy;

class PkTransaction : public TQObject
{
    TQ_OBJECT
public:
    explicit PkTransaction(TQObject *parent = nullptr, const char *name = nullptr);
    ~PkTransaction();

    void searchNames(Pk::Bitfield filters, const TQStringList &terms);
    void searchDetails(Pk::Bitfield filters, const TQStringList &terms);
    void searchFiles(Pk::Bitfield filters, const TQStringList &terms);
    void searchGroups(Pk::Bitfield filters, const TQStringList &groups);
    void resolve(Pk::Bitfield filters, const TQStringList &names);
    void getPackages(Pk::Bitfield filters);
    void getDetails(const TQStringList &ids);
    void getFiles(const TQStringList &ids);
    void dependsOn(Pk::Bitfield filters, const TQStringList &ids, bool recursive);
    void requiredBy(Pk::Bitfield filters, const TQStringList &ids, bool recursive);
    void getUpdates(Pk::Bitfield filters);
    void getUpdateDetail(const TQStringList &ids);
    void getRepoList(Pk::Bitfield filters);
    void repoEnable(const TQString &repoId, bool enabled);
    void getOldTransactions(uint count);
    void refreshCache(bool force);
    void installPackages(Pk::Bitfield flags, const TQStringList &ids);
    void removePackages(Pk::Bitfield flags, const TQStringList &ids, bool allowDeps, bool autoremove);
    void updatePackages(Pk::Bitfield flags, const TQStringList &ids);
    void installFiles(Pk::Bitfield flags, const TQStringList &paths);
    void installSignature(uint type, const TQString &keyId, const TQString &packageId);
    void acceptEula(const TQString &eulaId);

    /* Live properties, updated before statusChanged() and progressChanged(). */
    uint status() const { return m_status; }
    uint percentage() const { return m_percentage; }
    bool allowCancel() const { return m_allowCancel; }
    TQString path() const { return m_path; }
    bool isFinished() const { return m_finished; }

public slots:
    void cancel();

signals:
    void package(const PkPackage &package);
    void details(const PkDetails &details);
    void files(const TQString &packageId, const TQStringList &files);
    void updateDetail(const PkUpdateDetail &detail);
    void repoDetail(const PkRepo &repo);
    void historyEntry(const PkHistoryEntry &entry);
    void itemProgress(const TQString &id, uint status, uint percentage);
    void errorCode(uint code, const TQString &details);
    void signatureRequired(const PkSignature &signature);
    void eulaRequired(const PkEula &eula);
    void restartRequired(uint type, const TQString &packageId);
    void statusChanged(uint status);
    void progressChanged(uint percentage);
    void allowCancelChanged(bool allowed);
    void finished(uint exit, uint runtime);

private slots:
    void onDaemonReply(int callId, const TQT_DBusMessage &message);
    void onMethodReply(int callId, const TQT_DBusMessage &message);
    void onSignal(const TQT_DBusMessage &message);
    void onDaemonLost();

private:
    void start(const TQString &method, const TQValueList<TQT_DBusData> &params);
    void fail(uint code, const TQString &details);
    void finish(uint exit, uint runtime);
    void readProperties(const TQT_DBusMessage &message);
    void readChanged(const TQT_DBusMessage &message);
    void applyProperty(const TQString &key, const TQT_DBusData &value);
    void releaseMatch();

    TQT_DBusProxy *m_daemonProxy;
    TQT_DBusProxy *m_proxy;
    TQT_DBusProxy *m_signalProxy;
    TQString m_path;
    TQString m_match;
    TQString m_method;
    TQValueList<TQT_DBusData> m_params;
    int m_createCall;
    int m_methodCall;
    uint m_status;
    uint m_percentage;
    bool m_allowCancel;
    bool m_cancelPending;
    bool m_finished;
    bool m_started;
};

#endif
