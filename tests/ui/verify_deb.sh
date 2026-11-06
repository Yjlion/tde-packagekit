#!/bin/bash
# Installs the built package and checks what it put on the system. The app that
# runs is the packaged one; only the mime-offers test helper is from the build tree.
set -u
. /src/tests/ui/session.sh
FAILS=0
pass() { echo "ok   - $1"; }
fail() { echo "FAIL - $1"; FAILS=$((FAILS + 1)); }
check() { local d=$1; shift; if "$@"; then pass "$d"; else fail "$d"; fi; }

DEB=$(ls /src/dist/tdepackagekit-trinity_*.deb | head -1)
[ -n "$DEB" ] || { echo "no package in dist/; run ./dk deb first"; exit 1; }

# Every dependency must be resolvable from the configured repositories.
check "dependencies resolve" apt-get install -s "$DEB"

sudo /usr/bin/dpkg -i "$DEB" >/tmp/dpkg.log 2>&1 || { tail -5 /tmp/dpkg.log; fail "package installs"; }
check "package installs" dpkg -s tdepackagekit-trinity

P=/opt/trinity
check "application binary"      test -x $P/bin/tdepackagekit
check "notifier binary"         test -x $P/bin/tdepackagekit-notifier
check "menu entry"              test -f $P/share/applications/tde/tdepackagekit.desktop
check "file handler entry"      grep -q application/x-deb $P/share/applications/tde/tdepackagekit-install-file.desktop
check "autostart entry"         test -f $P/share/autostart/tdepackagekit-notifier.desktop
check "icon"                    test -f $P/share/icons/crystalsvg/48x48/apps/tdepackagekit.png
check "handbook"                test -f $P/share/doc/tde/HTML/en/tdepackagekit/index.cache.bz2
check "handbook screenshots"    test -f $P/share/doc/tde/HTML/en/tdepackagekit/browse-details.png
check "handbook in the menu entry" grep -q '^X-DocPath=tdepackagekit/index.html' $P/share/applications/tde/tdepackagekit.desktop
man_page() { compgen -G "$P/share/man/man1/$1.1*" >/dev/null; }
check "man page"                man_page tdepackagekit
check "notifier man page"       man_page tdepackagekit-notifier
check "no test programs shipped" test ! -e $P/bin/pkcore-cli

ldd $P/bin/tdepackagekit | grep -q 'not found' && fail "all libraries found" || pass "all libraries found"

ui_session_start
trap 'ui_session_stop' EXIT
SERVICES_STARTED=0
sudo /src/docker/test-services.sh start allow && SERVICES_STARTED=1
$P/bin/tdepackagekit --nofork >/tmp/app.log 2>&1 &
ui_wait_window "Software Management" 30 >/dev/null && pass "packaged application opens" || fail "packaged application opens"
kbuildsycoca --noincremental >/dev/null 2>&1
sleep 1
# To a file: the lookup can start a TDE daemon, which would hold a $(...) pipe open.
timeout 20 /build/tree/tests/integration/mime-offers application/x-deb >/tmp/offers 2>/dev/null </dev/null
OFFERS=$(cat /tmp/offers)
echo "$OFFERS" | grep -q tdepackagekit-install-file && pass "offered for .deb files" || { echo "offers: $OFFERS"; fail "offered for .deb files"; }

# A login: Trinity's launcher runs the autostart phases, and the notifier must start
# and keep running, with its tray icon shown.
# The launcher runs it by bare name, and process names are cut to 15 characters.
notifier_running() { pgrep -f '(^|/)tdepackagekit-notifier( |$)' >/dev/null; }
waitfor() { local i; for i in $(seq 1 50); do "$@" && return 0; sleep 0.2; done; return 1; }
pkill -x tdepackagekit >/dev/null 2>&1 || true
mkdir -p "$HOME/.trinity/share/config"
printf '[Notifier]\nInitialDelaySeconds=600\n' > "$HOME/.trinity/share/config/tdepackagekitrc"
timeout 5 dcop tdelauncher >/dev/null 2>&1 || tdeinit </dev/null >/tmp/tdeinit.log 2>&1
waitfor eval 'timeout 2 dcop tdelauncher >/dev/null 2>&1'
for phase in 0 1 2; do timeout 10 dcop tdelauncher tdelauncher autoStart "$phase" >/dev/null 2>&1; sleep 1; done
waitfor notifier_running && pass "notifier starts at login" || fail "notifier starts at login"
sleep 3
notifier_running && pass "notifier keeps running" || fail "notifier keeps running"
# There is no Kicker to embed the icon, but showing it gives the tray window icon size.
tray_shown() { xwininfo -root -tree | grep '"Software Update Notifier": ("tdepackagekit-notifier"' | grep -q ' 22x22+'; }
waitfor tray_shown && pass "notifier shows its tray icon" || fail "notifier shows its tray icon"
pkill -f '(^|/)tdepackagekit-notifier( |$)' || true

# Turned off in the settings, it exits straight away.
printf '[Notifier]\nEnabled=false\n' > "$HOME/.trinity/share/config/tdepackagekitrc"
timeout 20 $P/bin/tdepackagekit-notifier --nofork >/dev/null 2>&1 </dev/null
[ $? -ne 124 ] && pass "notifier stays off when turned off" || fail "notifier stays off when turned off"

echo "package checks: $FAILS failure(s)"
exit "$((FAILS > 0))"
