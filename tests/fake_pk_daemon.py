#!/usr/bin/env python3
"""A scripted stand-in for packagekitd, for cases apt can't produce on demand.

It owns org.freedesktop.PackageKit on a private bus. The client reaches it by
setting DBUS_SYSTEM_BUS_ADDRESS. What a transaction does depends on the name of
the package it is given, and every method call is appended to the log file so
that tests can check what the client really asked for.

  fake-eula     needs a license accepted       fake-key     needs a key trusted
  fake-restart  asks for a system restart      fake-fail    fails with an error
  fake-crash    the daemon dies mid-operation  fake-slow    slow, can be cancelled
  fake-untrust  refused while ONLY_TRUSTED     fake-deps    pulls in dependencies
"""
import argparse
import sys

import dbus
import dbus.mainloop.glib
import dbus.service
from gi.repository import GLib

BUS_NAME = "org.freedesktop.PackageKit"
PATH = "/org/freedesktop/PackageKit"
IFACE = "org.freedesktop.PackageKit"
TX_IFACE = "org.freedesktop.PackageKit.Transaction"
PROPS = "org.freedesktop.DBus.Properties"

# Values from pk-enum.h, which the unit tests check against the real header.
INFO_INSTALLED, INFO_AVAILABLE, INFO_LOW, INFO_ENHANCEMENT, INFO_NORMAL = 1, 2, 3, 4, 5
INFO_BUGFIX, INFO_IMPORTANT, INFO_SECURITY, INFO_BLOCKED = 6, 7, 8, 9
INFO_DOWNLOADING, INFO_UPDATING, INFO_INSTALLING, INFO_REMOVING = 10, 11, 12, 13
INFO_FINISHED, INFO_CRITICAL = 18, 26
EXIT_SUCCESS, EXIT_FAILED, EXIT_CANCELLED, EXIT_KEY, EXIT_EULA = 1, 2, 3, 4, 5
EXIT_NEED_UNTRUSTED = 8
STATUS_SETUP, STATUS_RUNNING, STATUS_REMOVE, STATUS_REFRESH = 2, 3, 6, 7
STATUS_DOWNLOAD, STATUS_INSTALL, STATUS_UPDATE, STATUS_FINISHED = 8, 9, 10, 18
ERR_NOT_AUTHORIZED, ERR_NO_LICENSE, ERR_GPG, ERR_TX = 48, 34, 5, 16
RESTART_SYSTEM = 4
FLAG_ONLY_TRUSTED, FLAG_SIMULATE = 1 << 1, 1 << 2
ROLE_INSTALL, ROLE_REMOVE, ROLE_UPDATE, ROLE_REFRESH = 11, 14, 22, 13


def pid(name, version="1.0", arch="amd64", data="fake-main"):
    return f"{name};{version};{arch};{data}"


CATALOG = [
    (INFO_AVAILABLE, pid("fake-editor", "3.2"), "A plain text editor"),
    (INFO_INSTALLED, pid("fake-browser", "98.0", data="manual:fake-main"), "A web browser"),
    (INFO_AVAILABLE, pid("fake-mail", "2.1"), "An email client"),
    (INFO_AVAILABLE, pid("fake-eula"), "Package with a license agreement"),
    (INFO_AVAILABLE, pid("fake-key"), "Package signed with an unknown key"),
    (INFO_AVAILABLE, pid("fake-restart"), "Package that needs a restart"),
    (INFO_AVAILABLE, pid("fake-fail"), "Package that fails to install"),
    (INFO_AVAILABLE, pid("fake-crash"), "Package that stops the daemon"),
    (INFO_AVAILABLE, pid("fake-slow"), "Package that takes a while"),
    (INFO_AVAILABLE, pid("fake-untrust"), "Package from an unverified source"),
    (INFO_AVAILABLE, pid("fake-deps"), "Package with dependencies"),
]

UPDATES = [
    (INFO_CRITICAL, pid("fake-kernel", "6.1.2"), "The Linux kernel"),
    (INFO_SECURITY, pid("fake-openssl", "3.0.9"), "Secure communication library"),
    (INFO_SECURITY, pid("fake-browser", "99.0"), "A web browser"),
    (INFO_IMPORTANT, pid("fake-glibc", "2.41-2"), "The GNU C library"),
    (INFO_BUGFIX, pid("fake-editor", "3.2.1"), "A plain text editor"),
    (INFO_NORMAL, pid("fake-mail", "2.2"), "An email client"),
    (INFO_ENHANCEMENT, pid("fake-themes", "1.4"), "Extra desktop themes"),
    (INFO_BLOCKED, pid("fake-driver", "5.0"), "A graphics driver"),
]

REPOS = [
    ("fake-main", "Fake Linux main repository", True),
    ("fake-security", "Fake Linux security updates", True),
    ("fake-backports", "Fake Linux backports", False),
]


class Daemon(dbus.service.Object):
    def __init__(self, bus, args):
        super().__init__(bus, PATH)
        self.bus = bus
        self.args = args
        self.count = 0
        self.eula_ok = False
        self.key_ok = False
        self.transactions = []
        self.repos = {r[0]: r[2] for r in REPOS}

    def log(self, text):
        with open(self.args.log, "a", encoding="utf-8") as handle:
            handle.write(text + "\n")

    def properties(self):
        roles = sum(1 << r for r in range(1, 34))
        # Administration, Games, Graphics, Internet, Office, System and more.
        groups = sum(1 << g for g in (3, 11, 12, 13, 17, 19, 22, 27))
        filters = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 6) | (1 << 7) | (1 << 16)
        return dbus.Dictionary({
            "VersionMajor": dbus.UInt32(1, variant_level=1),
            "VersionMinor": dbus.UInt32(3, variant_level=1),
            "VersionMicro": dbus.UInt32(1, variant_level=1),
            "BackendName": dbus.String("fake", variant_level=1),
            "BackendDescription": dbus.String("Scripted test backend", variant_level=1),
            "BackendAuthor": dbus.String("tests", variant_level=1),
            "Roles": dbus.UInt64(roles, variant_level=1),
            "Groups": dbus.UInt64(groups, variant_level=1),
            "Filters": dbus.UInt64(filters, variant_level=1),
            "MimeTypes": dbus.Array(["application/x-deb"], signature="s", variant_level=1),
            "Locked": dbus.Boolean(False, variant_level=1),
            "NetworkState": dbus.UInt32(2, variant_level=1),
            "DistroId": dbus.String("fake;1;x86_64", variant_level=1),
        }, signature="sv")

    @dbus.service.method(PROPS, in_signature="s", out_signature="a{sv}")
    def GetAll(self, interface):
        return self.properties()

    @dbus.service.method(IFACE, out_signature="o")
    def CreateTransaction(self):
        self.count += 1
        tx = Transaction(self, f"/{self.count}_fake")
        self.transactions.append(tx)
        return tx.path

    @dbus.service.method(IFACE, in_signature="u", out_signature="u")
    def GetTimeSinceAction(self, role):
        return 3 * 3600

    @dbus.service.signal(IFACE)
    def UpdatesChanged(self):
        pass

    @dbus.service.signal(IFACE)
    def RepoListChanged(self):
        pass

    @dbus.service.signal(IFACE, signature="as")
    def TransactionListChanged(self, paths):
        pass


class Transaction(dbus.service.Object):
    def __init__(self, daemon, path):
        super().__init__(daemon.bus, path)
        self.daemon = daemon
        self.path = path
        self.cancelled = False
        self.timers = []
        self.done = False

    # Helpers ------------------------------------------------------------------

    def after(self, ms, func, *args, **kwargs):
        def fire():
            if not self.done:
                func(*args, **kwargs)
            return False
        self.timers.append(GLib.timeout_add(ms, fire))

    def props(self, **values):
        changed = {}
        for key, value in values.items():
            if key == "AllowCancel":
                changed[key] = dbus.Boolean(value, variant_level=1)
            else:
                changed[key] = dbus.UInt32(value, variant_level=1)
        self.PropertiesChanged(TX_IFACE, changed, dbus.Array([], signature="s"))

    def finish(self, exit_code=EXIT_SUCCESS):
        self.props(Status=STATUS_FINISHED, Percentage=100, AllowCancel=False)
        self.done = True
        self.Finished(exit_code, 10)
        self.Destroy()

    def emit_packages(self, items):
        if self.daemon.args.legacy_signals:
            for info, package_id, summary in items:
                self.Package(info, package_id, summary)
        else:
            self.Packages(dbus.Array(
                [dbus.Struct((dbus.UInt32(i), p, s), signature="uss") for i, p, s in items],
                signature="(uss)"))

    def query(self, items):
        self.after(50, self.props, Status=STATUS_RUNNING)
        self.after(100, self.emit_packages, items)
        self.after(150, self.finish)

    def log_call(self, method, *values):
        self.daemon.log(" ".join([method] + [str(v) for v in values]))

    # Methods ------------------------------------------------------------------

    @dbus.service.method(TX_IFACE, in_signature="as")
    def SetHints(self, hints):
        self.log_call("SetHints", ",".join(hints))

    @dbus.service.method(TX_IFACE)
    def Cancel(self):
        self.log_call("Cancel")
        self.cancelled = True
        self.after(100, self.finish, EXIT_CANCELLED)

    @dbus.service.method(TX_IFACE, in_signature="tas")
    def SearchNames(self, filters, values):
        self.log_call("SearchNames", filters, ",".join(values))
        self.query([c for c in CATALOG if any(v in c[1] for v in values)])

    @dbus.service.method(TX_IFACE, in_signature="tas")
    def SearchDetails(self, filters, values):
        self.log_call("SearchDetails", filters, ",".join(values))
        self.query([c for c in CATALOG if any(v in c[2].lower() for v in values)])

    @dbus.service.method(TX_IFACE, in_signature="tas")
    def SearchFiles(self, filters, values):
        self.log_call("SearchFiles", filters, ",".join(values))
        self.query(CATALOG[:1])

    @dbus.service.method(TX_IFACE, in_signature="tas")
    def SearchGroups(self, filters, values):
        self.log_call("SearchGroups", filters, ",".join(values))
        self.query(CATALOG[:3])

    @dbus.service.method(TX_IFACE, in_signature="as")
    def GetDetails(self, ids):
        self.log_call("GetDetails", ",".join(ids))
        def send():
            for package_id in ids:
                self.Details(dbus.Dictionary({
                    "package-id": dbus.String(package_id, variant_level=1),
                    "group": dbus.UInt32(13, variant_level=1),
                    "summary": dbus.String("Summary from GetDetails", variant_level=1),
                    "description": dbus.String(
                        "A longer description of this package.\nIt has two lines & <markup>.",
                        variant_level=1),
                    "url": dbus.String("https://example.invalid/project", variant_level=1),
                    "license": dbus.String("GPL-2.0-or-later", variant_level=1),
                    "size": dbus.UInt64(12345678, variant_level=1),
                }, signature="sv"))
        self.after(50, send)
        self.after(100, self.finish)

    @dbus.service.method(TX_IFACE, in_signature="as")
    def GetFiles(self, ids):
        self.log_call("GetFiles", ",".join(ids))
        self.after(50, self.Files, ids[0], ["/usr/bin/fake", "/usr/share/doc/fake/README"])
        self.after(100, self.finish)

    @dbus.service.method(TX_IFACE, in_signature="tasb")
    def DependsOn(self, filters, ids, recursive):
        self.log_call("DependsOn", ",".join(ids))
        self.query([(INFO_INSTALLED, pid("fake-libc", "2.41"), "C library"),
                    (INFO_AVAILABLE, pid("fake-libfoo", "1.2"), "Foo library")])

    @dbus.service.method(TX_IFACE, in_signature="tasb")
    def RequiredBy(self, filters, ids, recursive):
        self.log_call("RequiredBy", ",".join(ids))
        self.query([(INFO_INSTALLED, pid("fake-desktop", "4.0"), "Desktop meta package")])

    @dbus.service.method(TX_IFACE, in_signature="t")
    def GetUpdates(self, filters):
        self.log_call("GetUpdates", filters)
        self.query(UPDATES)

    @dbus.service.method(TX_IFACE, in_signature="as")
    def GetUpdateDetail(self, ids):
        self.log_call("GetUpdateDetail", ",".join(ids))
        rows = []
        for package_id in ids:
            name = package_id.split(";")[0]
            rows.append((package_id, [pid(name, "0.9")], [], ["https://example.invalid/advisory"],
                         ["https://bugs.example.invalid/1234"],
                         ["https://cve.example.invalid/CVE-2026-0001"],
                         RESTART_SYSTEM if name == "fake-kernel" else 1,
                         f"This update fixes a problem in {name}.",
                         "* Fixed a crash\n* Fixed a memory leak", 1,
                         "2026-10-01T10:00:00Z", "2026-10-02T10:00:00Z"))
        def send():
            if self.daemon.args.legacy_signals:
                for row in rows:
                    self.UpdateDetail(*row)
            else:
                self.UpdateDetails(dbus.Array(
                    [dbus.Struct(r, signature="sasasasasasussuss") for r in rows],
                    signature="(sasasasasasussuss)"))
        self.after(50, send)
        self.after(100, self.finish)

    @dbus.service.method(TX_IFACE, in_signature="t")
    def GetRepoList(self, filters):
        self.log_call("GetRepoList", filters)
        def send():
            for repo_id, description, _ in REPOS:
                self.RepoDetail(repo_id, description, self.daemon.repos[repo_id])
        self.after(50, send)
        self.after(100, self.finish)

    @dbus.service.method(TX_IFACE, in_signature="sb")
    def RepoEnable(self, repo_id, enabled):
        self.log_call("RepoEnable", repo_id, bool(enabled))
        if repo_id == "fake-security":
            self.after(50, self.ErrorCode, 54, "This repository is protected.")
            self.after(100, self.finish, EXIT_FAILED)
            return
        self.daemon.repos[repo_id] = bool(enabled)
        self.after(50, self.finish)

    @dbus.service.method(TX_IFACE, in_signature="u")
    def GetOldTransactions(self, number):
        self.log_call("GetOldTransactions", number)
        def send():
            self.Transaction("/old/1", "2026-11-01T09:15:00Z", True, ROLE_INSTALL, 4200,
                             "installing\t" + pid("fake-editor", "3.2"), 1000, "tdepackagekit")
            self.Transaction("/old/2", "2026-11-02T18:40:00Z", False, ROLE_UPDATE, 1200,
                             "updating\t" + pid("fake-kernel", "6.1.1") + "\nupdating\t" + pid("fake-mail", "2.1"),
                             1000, "tdepackagekit")
            self.Transaction("/old/3", "2026-11-03T11:05:00Z", True, ROLE_REMOVE, 800,
                             "removing\t" + pid("fake-games", "1.0"), 1000, "tdepackagekit")
            self.Transaction("/old/4", "2026-11-04T07:00:00Z", True, ROLE_REFRESH, 300, "", 1000, "notifier")
        self.after(50, send)
        self.after(100, self.finish)

    @dbus.service.method(TX_IFACE, in_signature="b")
    def RefreshCache(self, force):
        self.log_call("RefreshCache", bool(force))
        self.after(50, self.props, Status=STATUS_REFRESH, Percentage=50)
        self.after(150, self.finish)

    @dbus.service.method(TX_IFACE, in_signature="s")
    def AcceptEula(self, eula_id):
        self.log_call("AcceptEula", eula_id)
        self.daemon.eula_ok = True
        self.after(50, self.finish)

    @dbus.service.method(TX_IFACE, in_signature="uss")
    def InstallSignature(self, sig_type, key_id, package_id):
        self.log_call("InstallSignature", sig_type, key_id, package_id)
        self.daemon.key_ok = True
        self.after(50, self.finish)

    @dbus.service.method(TX_IFACE, in_signature="tas")
    def InstallPackages(self, flags, ids):
        self.log_call("InstallPackages", flags, ",".join(ids))
        self.change(flags, ids, INFO_INSTALLING, STATUS_INSTALL)

    @dbus.service.method(TX_IFACE, in_signature="tas")
    def InstallFiles(self, flags, paths):
        self.log_call("InstallFiles", flags, ",".join(paths))
        self.change(flags, [pid("fake-local")], INFO_INSTALLING, STATUS_INSTALL)

    @dbus.service.method(TX_IFACE, in_signature="tasbb")
    def RemovePackages(self, flags, ids, allow_deps, autoremove):
        self.log_call("RemovePackages", flags, ",".join(ids), bool(allow_deps), bool(autoremove))
        self.change(flags, ids, INFO_REMOVING, STATUS_REMOVE)

    @dbus.service.method(TX_IFACE, in_signature="tas")
    def UpdatePackages(self, flags, ids):
        self.log_call("UpdatePackages", flags, ",".join(ids))
        self.change(flags, ids, INFO_UPDATING, STATUS_UPDATE)

    def change(self, flags, ids, info, status):
        names = [i.split(";")[0] for i in ids]
        simulate = bool(flags & FLAG_SIMULATE)
        self.after(20, self.props, AllowCancel=True, Status=STATUS_SETUP)

        if simulate:
            items = [(info, i, "Requested package") for i in ids]
            if "fake-deps" in names:
                items.append((INFO_INSTALLING, pid("fake-libfoo", "1.2"), "Foo library, a new dependency"))
                items.append((INFO_REMOVING, pid("fake-oldfoo", "0.9"), "Old Foo library, now in conflict"))
            self.after(50, self.emit_packages, items)
            self.after(100, self.finish)
            return

        if "fake-crash" in names:
            self.after(200, sys.exit, 0)
            return
        if "fake-fail" in names:
            self.after(100, self.ErrorCode, ERR_TX, "dpkg: error processing package fake-fail")
            self.after(150, self.finish, EXIT_FAILED)
            return
        if "fake-eula" in names and not self.daemon.eula_ok:
            self.after(50, self.EulaRequired, "fake-eula-1", ids[0], "Fake Software Inc.",
                       "FAKE END USER LICENSE AGREEMENT\n\nYou may use this software for testing.\n"
                       "There is no warranty of any kind.")
            self.after(70, self.ErrorCode, ERR_NO_LICENSE, "A license must be accepted.")
            self.after(100, self.finish, EXIT_EULA)
            return
        if "fake-key" in names and not self.daemon.key_ok:
            self.after(50, self.RepoSignatureRequired, ids[0], "fake-thirdparty",
                       "https://example.invalid/key.asc", "Fake Third Party <keys@example.invalid>",
                       "ABCD1234", "0123 4567 89AB CDEF 0123  4567 89AB CDEF ABCD 1234",
                       "2026-01-15", 1)
            self.after(70, self.ErrorCode, ERR_GPG, "The repository key is not trusted.")
            self.after(100, self.finish, EXIT_KEY)
            return
        if "fake-untrust" in names and flags & FLAG_ONLY_TRUSTED:
            self.after(50, self.ErrorCode, 31, "The package is not signed.")
            self.after(100, self.finish, EXIT_NEED_UNTRUSTED)
            return

        steps = 40 if "fake-slow" in names else 4
        delay = 150 if "fake-slow" in names else 40
        self.after(30, self.props, Status=STATUS_DOWNLOAD, Percentage=0)
        for n in range(steps):
            package = ids[n % len(ids)]
            self.after(50 + n * delay, self.Package, info, package, "In progress")
            self.after(50 + n * delay, self.props, Percentage=int(100 * n / steps), Status=status)
        end = 60 + steps * delay
        if "fake-restart" in names:
            self.after(end, self.RequireRestart, RESTART_SYSTEM, ids[0])
        self.after(end + 10, self.finish)
        self.after(end + 20, self.daemon.UpdatesChanged)

    # Signals ------------------------------------------------------------------

    @dbus.service.signal(PROPS, signature="sa{sv}as")
    def PropertiesChanged(self, interface, changed, invalidated):
        pass

    @dbus.service.signal(TX_IFACE, signature="uss")
    def Package(self, info, package_id, summary):
        pass

    @dbus.service.signal(TX_IFACE, signature="a(uss)")
    def Packages(self, packages):
        pass

    @dbus.service.signal(TX_IFACE, signature="a{sv}")
    def Details(self, data):
        pass

    @dbus.service.signal(TX_IFACE, signature="sas")
    def Files(self, package_id, files):
        pass

    @dbus.service.signal(TX_IFACE, signature="sasasasasasussuss")
    def UpdateDetail(self, package_id, updates, obsoletes, vendor, bugzilla, cve, restart,
                     text, changelog, state, issued, updated):
        pass

    @dbus.service.signal(TX_IFACE, signature="a(sasasasasasussuss)")
    def UpdateDetails(self, details):
        pass

    @dbus.service.signal(TX_IFACE, signature="ssb")
    def RepoDetail(self, repo_id, description, enabled):
        pass

    @dbus.service.signal(TX_IFACE, signature="osbuusus")
    def Transaction(self, path, timespec, succeeded, role, duration, data, uid, cmdline):
        pass

    @dbus.service.signal(TX_IFACE, signature="us")
    def ErrorCode(self, code, details):
        pass

    @dbus.service.signal(TX_IFACE, signature="sssssssu")
    def RepoSignatureRequired(self, package_id, repo, url, userid, key_id, fingerprint, timestamp, kind):
        pass

    @dbus.service.signal(TX_IFACE, signature="ssss")
    def EulaRequired(self, eula_id, package_id, vendor, text):
        pass

    @dbus.service.signal(TX_IFACE, signature="us")
    def RequireRestart(self, restart, package_id):
        pass

    @dbus.service.signal(TX_IFACE, signature="uu")
    def Finished(self, exit_code, runtime):
        pass

    @dbus.service.signal(TX_IFACE)
    def Destroy(self):
        pass


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--log", required=True)
    parser.add_argument("--legacy-signals", action="store_true",
                        help="send Package and UpdateDetail rather than the batched signals")
    args = parser.parse_args()

    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    bus = dbus.SystemBus()
    name = dbus.service.BusName(BUS_NAME, bus)
    daemon = Daemon(bus, args)
    open(args.log, "w", encoding="utf-8").close()
    print("fake daemon ready", flush=True)
    GLib.MainLoop().run()
    del name, daemon


if __name__ == "__main__":
    main()
