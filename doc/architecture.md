# Architecture

```
tdepackagekit             tdepackagekit-notifier
(main window, pages,      (tray icon, popups)
 dialogs, apply flow)
        \                     /
         \                   /
          libtdepkcore (static, no widgets)
          PkDaemon, PkTransaction, PkEnums,
          PackageId, ChangeQueue, TdePkSettings
                    |
              libdbus-1-tqt
                    |  system bus
              packagekitd  ---> polkit (authorization)
                    |
              apt backend
```

The app never runs as root. PackageKit does the privileged work, and asks polkit whether the user
may do it.

## Why D-Bus directly

PackageKit has client libraries for GLib, Qt 5 and Qt 6, but TQt3 can't use the Qt ones, and the GLib
one would bring GObject into a TQt application. So `src/pk` is a small layer over `libdbus-1-tqt`.

## Core library (`src/pk`)

**`PkDaemon`** is a singleton for the daemon's properties (backend, supported roles, groups and
filters) and global signals (`UpdatesChanged`, `RepoListChanged`, and so on). It also watches
`NameOwnerChanged`, so it knows if the daemon goes away. The daemon exits when idle and is restarted
by D-Bus on demand, so that alone isn't an error.

**`PkTransaction`** wraps one transaction. Call one action, connect to its signals, and delete it
after `finished()`:

1. `CreateTransaction` on the daemon gives a transaction object path.
2. Adds a D-Bus match rule for that path, so the bus delivers its signals.
3. `SetHints` (locale, `interactive=true`), then the method itself.
4. Results arrive as signals; `finished()` is emitted **exactly once**, even if the call is refused,
   the service is missing or the daemon crashes.

**Never run a nested event loop from a `PkTransaction` or `PkDaemon` signal**, which rules out modal
dialogs and `KMessageBox`. `libdbus-1-tqt` holds a list iterator while it emits each D-Bus signal.
A nested loop re-enters that code and frees the node, so the app crashes when the dialog closes.
It also runs any pending `deleteLater()` while the sender is still emitting. Leave the callback first
with `TQTimer::singleShot(0, ...)`, as `ApplyFlow::onFinished()` does.

Signal details worth knowing:

- PackageKit 1.3 has both single (`Package`, `UpdateDetail`) and batched (`Packages`,
  `UpdateDetails`) signals. Both are handled and reach the app as the same `package()` and
  `updateDetail()` signals.
- Progress comes through `org.freedesktop.DBus.Properties.PropertiesChanged`, a different interface
  from the transaction's. So signals are received by a proxy with no interface filter.
- Signals carry the sender's unique bus name, so the signal proxy doesn't filter on the service.

**`PkEnums`** mirrors PackageKit's enums and holds user-visible labels. A test compares the values
with `pk-enum.h` at compile time.

**`ChangeQueue`** holds pending changes, shared by the Browse and Updates pages.

## Applying changes (`ApplyFlow`)

```
simulate each step ---> confirm if needed ---> run each step (ONLY_TRUSTED) ---> restart prompt
                                                   |
                 license or key request: prompt, tell the daemon, retry the step
                 needs untrusted: warn, retry without ONLY_TRUSTED
                 error: report, stop, keep the remaining changes queued
```

Removals run first, then installs, then updates, each as its own transaction. A step that succeeds
leaves the queue at once, so a later failure leaves only what is still undone.

## Pages (`src/app`)

Each page owns its transactions. When a new request replaces a running one (typing a new search,
say), `QueryHelper::abandon()` disconnects the old transaction, cancels it, and lets it delete
itself when it ends. Late results can't reach the page.

Search results are buffered and added every 200 ms with sorting off, so a large result set
doesn't sort on each insert.

## Notifier (`src/notifier`)

A `KSystemTray` that checks when due and on `UpdatesChanged`. It refreshes with `force=false`. It has
no code that installs anything, and a test confirms it never calls an install method. The settings
dialog tells it to reload through DCOP.

## Settings

`TdePkSettings` reads and writes `tdepackagekitrc`, in the `[Notifier]` group.
