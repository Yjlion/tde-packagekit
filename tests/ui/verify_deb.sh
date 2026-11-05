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

echo "package checks: $FAILS failure(s)"
exit "$((FAILS > 0))"
