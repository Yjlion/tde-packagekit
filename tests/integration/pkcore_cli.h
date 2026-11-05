/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef PKCORE_CLI_H
#define PKCORE_CLI_H

#include <stdio.h>

#include <tqapplication.h>
#include <tqobject.h>
#include <tqstringlist.h>

#include "pkdaemon.h"
#include "pktransaction.h"

static inline void out(const TQString &line)
{
    printf("%s\n", line.utf8().data());
    fflush(stdout);
}

class Driver : public TQObject
{
    TQ_OBJECT
public:
    Driver(PkTransaction *t) {
        connect(t, TQ_SIGNAL(package(const PkPackage &)), TQ_SLOT(package(const PkPackage &)));
        connect(t, TQ_SIGNAL(details(const PkDetails &)), TQ_SLOT(details(const PkDetails &)));
        connect(t, TQ_SIGNAL(files(const TQString &, const TQStringList &)), TQ_SLOT(files(const TQString &, const TQStringList &)));
        connect(t, TQ_SIGNAL(updateDetail(const PkUpdateDetail &)), TQ_SLOT(updateDetail(const PkUpdateDetail &)));
        connect(t, TQ_SIGNAL(repoDetail(const PkRepo &)), TQ_SLOT(repoDetail(const PkRepo &)));
        connect(t, TQ_SIGNAL(historyEntry(const PkHistoryEntry &)), TQ_SLOT(historyEntry(const PkHistoryEntry &)));
        connect(t, TQ_SIGNAL(errorCode(uint, const TQString &)), TQ_SLOT(errorCode(uint, const TQString &)));
        connect(t, TQ_SIGNAL(signatureRequired(const PkSignature &)), TQ_SLOT(signature(const PkSignature &)));
        connect(t, TQ_SIGNAL(eulaRequired(const PkEula &)), TQ_SLOT(eula(const PkEula &)));
        connect(t, TQ_SIGNAL(restartRequired(uint, const TQString &)), TQ_SLOT(restart(uint, const TQString &)));
        connect(t, TQ_SIGNAL(statusChanged(uint)), TQ_SLOT(status(uint)));
        connect(t, TQ_SIGNAL(progressChanged(uint)), TQ_SLOT(progress(uint)));
        connect(t, TQ_SIGNAL(finished(uint, uint)), TQ_SLOT(finished(uint, uint)));
    }
public slots:
    void package(const PkPackage &p) { out(TQString("package\t%1\t%2\t%3").arg(p.info).arg(p.id).arg(p.summary)); }
    void details(const PkDetails &d) { out(TQString("details\t%1\t%2\t%3\t%4\t%5").arg(d.id).arg(d.group).arg(d.license).arg(TQString::number(d.size)).arg(d.summary)); }
    void files(const TQString &id, const TQStringList &f) { out(TQString("files\t%1\t%2").arg(id).arg(f.join(","))); }
    void updateDetail(const PkUpdateDetail &d) { out(TQString("update\t%1\t%2\t%3\t%4\t%5").arg(d.id).arg(d.restart).arg(d.updates.join(",")).arg(d.cveUrls.join(",")).arg(d.updateText)); }
    void repoDetail(const PkRepo &r) { out(TQString("repo\t%1\t%2\t%3").arg(r.id).arg(r.enabled ? 1 : 0).arg(r.description)); }
    void historyEntry(const PkHistoryEntry &e) { out(TQString("history\t%1\t%2\t%3\t%4").arg(e.role).arg(e.succeeded ? 1 : 0).arg(e.timespec).arg(TQString(e.data).replace('\n', '|'))); }
    void errorCode(uint c, const TQString &d) { out(TQString("error\t%1\t%2").arg(c).arg(TQString(d).replace('\n', ' '))); }
    void signature(const PkSignature &s) { out(TQString("signature\t%1\t%2\t%3").arg(s.packageId).arg(s.keyId).arg(s.keyFingerprint)); }
    void eula(const PkEula &e) { out(TQString("eula\t%1\t%2\t%3").arg(e.id).arg(e.packageId).arg(e.vendor)); }
    void restart(uint t, const TQString &id) { out(TQString("restart\t%1\t%2").arg(t).arg(id)); }
    void status(uint s) { out(TQString("status\t%1").arg(s)); }
    void progress(uint p) { out(TQString("progress\t%1").arg(p)); }
    void finished(uint exit, uint) {
        out(TQString("finished\t%1").arg(exit));
        tqApp->exit(exit == Pk::ExitSuccess ? 0 : 3);
    }
};

class DaemonDriver : public TQObject
{
    TQ_OBJECT
public slots:
    void props() {
        PkDaemon *d = PkDaemon::self();
        out(TQString("daemon\tbackend=%1\tversion=%2\tlocked=%3").arg(d->backendName()).arg(d->version()).arg(d->locked()));
        out(TQString("roles\t%1").arg(TQString::number(d->roles())));
        out(TQString("groups\t%1").arg(TQString::number(d->groups())));
        tqApp->exit(0);
    }
    void gone(const TQString &why) { out("unavailable\t" + why); tqApp->exit(4); }
};

#endif
