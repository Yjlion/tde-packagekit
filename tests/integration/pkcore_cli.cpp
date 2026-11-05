/*
 * Command-line driver for libtdepkcore, used by the integration tests.
 * It prints one parseable line per signal and exits 0 only on ExitSuccess.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <stdio.h>
#include <stdlib.h>

#include <tqapplication.h>
#include <tqobject.h>
#include <tqstringlist.h>
#include <tqtimer.h>

#include <tdeaboutdata.h>
#include <tdecmdlineargs.h>
#include <tdeinstance.h>

#include "pkcore_cli.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <action> [args...]\n", argv[0]);
        return 2;
    }
    TDEAboutData about("pkcore-cli", "pkcore-cli", "1");
    TDEInstance instance(&about);
    TQApplication app(argc, argv, false);

    const TQString action = argv[1];
    TQStringList args;
    for (int i = 2; i < argc; ++i) args << TQString::fromUtf8(argv[i]);

    if (action == "daemon") {
        DaemonDriver d;
        TQObject::connect(PkDaemon::self(), TQ_SIGNAL(propertiesChanged()), &d, TQ_SLOT(props()));
        TQObject::connect(PkDaemon::self(), TQ_SIGNAL(unavailable(const TQString &)), &d, TQ_SLOT(gone(const TQString &)));
        PkDaemon::self()->refreshProperties();
        return app.exec();
    }

    PkTransaction t;
    Driver driver(&t);
    const Pk::Bitfield noFilter = Pk::bit(Pk::FilterNone);
    const Pk::Bitfield trusted = Pk::bit(Pk::TransactionFlagOnlyTrusted);
    const Pk::Bitfield simulate = trusted | Pk::bit(Pk::TransactionFlagSimulate);

    if (action == "search-name") t.searchNames(noFilter, args);
    else if (action == "search-details") t.searchDetails(noFilter, args);
    else if (action == "search-file") t.searchFiles(noFilter, args);
    else if (action == "search-group") t.searchGroups(noFilter, args);
    else if (action == "resolve") t.resolve(noFilter, args);
    else if (action == "details") t.getDetails(args);
    else if (action == "files") t.getFiles(args);
    else if (action == "depends") t.dependsOn(noFilter, args, false);
    else if (action == "required-by") t.requiredBy(noFilter, args, false);
    else if (action == "updates") t.getUpdates(noFilter);
    else if (action == "update-detail") t.getUpdateDetail(args);
    else if (action == "repos") t.getRepoList(noFilter);
    else if (action == "repo-enable") t.repoEnable(args[0], args[1] == "1");
    else if (action == "history") t.getOldTransactions(args.isEmpty() ? 20 : args[0].toUInt());
    else if (action == "refresh") t.refreshCache(true);
    else if (action == "install") t.installPackages(trusted, args);
    else if (action == "simulate-install") t.installPackages(simulate, args);
    else if (action == "remove") t.removePackages(trusted, args, true, false);
    else if (action == "simulate-remove") t.removePackages(simulate, args, true, false);
    else if (action == "update") t.updatePackages(trusted, args);
    else if (action == "simulate-update") t.updatePackages(simulate, args);
    else if (action == "install-file") t.installFiles(trusted, args);
    else if (action == "install-file-untrusted") t.installFiles(0, args);
    else if (action == "accept-eula") t.acceptEula(args[0]);
    else if (action == "install-signature") t.installSignature(Pk::SigTypeGpg, args[0], args[1]);
    else if (action == "cancel-after") {
        t.searchNames(noFilter, TQStringList("a"));
        TQTimer::singleShot(args[0].toInt(), &t, TQ_SLOT(cancel()));
    } else {
        fprintf(stderr, "unknown action %s\n", action.latin1());
        return 2;
    }
    return app.exec();
}

#include "pkcore_cli.moc"
