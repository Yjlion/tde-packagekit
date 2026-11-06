# Testing

`./dk test` runs everything in the container, in this order:

| Suite | Files | Covers |
|---|---|---|
| Unit | `tests/unit/test_core.cpp` | Package IDs, bitfields, severity order, group names, AppStream icon lookup, enums checked against `pk-enum.h` |
| Core, fake daemon | `tests/integration/test_core_fake.py` | Every signal, in batched and single forms; failures such as a daemon crash or no service |
| Core, real daemon | `tests/integration/test_core_real.py` | Real `packagekitd` and apt as a non-root user: search, install, update, remove, repos, history, refused authorization. Results are confirmed with `dpkg-query`. |
| UI, fake daemon | `tests/ui/test_ui_fake.sh` | The real window driven by `xdotool`: all pages, dialogs, prompts, app icons, a resizable error window and the notifier |
| UI, real daemon | `tests/ui/test_ui_real.sh` | End to end: tick, Apply, and the package is on disk |

`./dk verify-deb` is separate, and checks the built package: its files, the handbook and man pages,
the `.deb` file handler, and that Trinity's autostart starts the notifier.

## The fake daemon

`tests/fake_pk_daemon.py` owns `org.freedesktop.PackageKit` on a private bus, reached by setting
`DBUS_SYSTEM_BUS_ADDRESS`. A package's name decides what happens to it:

| Package | Behaviour |
|---|---|
| `fake-eula` | Needs a license accepted |
| `fake-key` | Needs a signing key trusted |
| `fake-untrust` | Refused under `ONLY_TRUSTED` |
| `fake-restart` | Asks for a system restart |
| `fake-fail` | Fails with an error |
| `fake-crash` | The daemon exits mid-operation |
| `fake-slow` | Takes several seconds, and can be cancelled |
| `fake-deps` | Pulls in a dependency and removes a package |

It logs every method call, so tests check what the client really sent, including the transaction
flags. Its enum numbers are written by hand: if PackageKit's change, the unit test catches our copy
in `src/pk`, but the fake's copy must be updated by hand.

## The real daemon

`docker/test-services.sh` starts the system bus, polkit and `packagekitd` by hand, because the
container has no systemd. `tests/fixtures/build-repo.sh` builds two local apt repositories, so that
switching from `v1` to `v2` produces a real update. This makes these tests quick and offline once the
image exists.

A test-only polkit rule lets the `dev` user act without an authentication agent. The refused-
authorization test swaps it for a rule that denies.

## UI tests

The UI tests run Xvfb, `dcopserver` and the `twin` window manager. Clicks use screen coordinates
for a window placed at the top-left corner, so layout changes can require updating the positions at the
top of `tests/ui/ui_lib.sh`. Screenshots of failures go to `/build/failures` in the build volume.

The tests set `MALLOC_PERTURB_` so that use-after-free bugs are more likely to fail, and `app_stop`
fails the test if the app died or its crash handler is showing.

TDE's accelerator manager can change a dialog's Alt-key shortcuts from what the source says. If a key
press hits the wrong button, check a screenshot for what is underlined.

## Not covered

These need a real Trinity session, so check them by hand after installing the package:

- The tray icon appears in Kicker's system tray, and left and right clicks work on it
- A real polkit password prompt from `polkit-agent-tde` appears, and works
- The notifier starts at a real login (`verify-deb` only runs the launcher's autostart phases)
- A real apt repository with an unknown signing key (only the fake daemon exercises the key prompt)
- A real package that requests a license or a restart (also only the fake daemon)
- Opening a `.deb` from Konqueror
