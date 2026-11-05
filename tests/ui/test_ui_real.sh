#!/bin/bash
# The whole path, end to end: real window, real packagekitd, real dpkg.
set -u
. /src/tests/ui/ui_lib.sh

sudo /usr/bin/dpkg --purge tdepk-test-hello tdepk-test-app tdepk-test-lib tdepk-test-local >/dev/null 2>&1
sudo /src/docker/test-services.sh polkit-rule allow
sudo /src/docker/test-services.sh use-fixture-repo v1
sudo /src/docker/test-services.sh restart-packagekit

ui_session_start
trap 'ui_session_stop' EXIT
app_start

# Rows sort by name: app, hello, lib.
search_for tdepk-test
click $ROW2_BOX
press_apply
for _ in $(seq 1 30); do is_installed tdepk-test-hello 1.0 && break; sleep 1; done
check "install: tick, Apply, package on disk" is_installed tdepk-test-hello 1.0

# A dependency needs confirmation.
sleep 2
search_for tdepk-test
click $ROW1_BOX
press_apply
ui_wait_window "Confirm Changes" 20 >/dev/null && pass "a dependency asks for confirmation" || fail "a dependency asks for confirmation"
key alt+c
for _ in $(seq 1 30); do is_installed tdepk-test-app 1.0 && break; sleep 1; done
check "install with dependency: app" is_installed tdepk-test-app 1.0
check "install with dependency: lib" is_installed tdepk-test-lib 1.0

# Remove by unticking.
sleep 2
search_for tdepk-test-hello
click $ROW1_BOX
press_apply
ui_wait_window "Confirm Changes" 20 >/dev/null && pass "removal asks first" || fail "removal asks first"
key alt+c
for _ in $(seq 1 30); do is_absent tdepk-test-hello && break; sleep 1; done
check "remove: untick, Apply, package gone" is_absent tdepk-test-hello
app_stop

# Update: install 1.0, publish 1.1, and apply it from the Updates page.
sudo /usr/bin/dpkg -i /build/fixture-repo/v1/tdepk-test-hello_1.0_all.deb >/dev/null
sudo /src/docker/test-services.sh use-fixture-repo v2
sudo /src/docker/test-services.sh restart-packagekit
app_start --updates
sleep 2
press_apply
for _ in $(seq 1 30); do is_installed tdepk-test-hello 1.1 && break; sleep 1; done
check "update: Updates page, Apply, new version on disk" is_installed tdepk-test-hello 1.1
app_stop

# A local package file, through the unverified prompt.
app_start --install-file /build/fixture-repo/tdepk-test-local_1.0_all.deb
DIALOG=$(ui_wait_window "Confirm Changes" 20) && pass "file: shows what it will do" || fail "file: shows what it will do"
# The dialog can open before app_start focuses the main window, so focus it here.
xdotool windowfocus --sync "$DIALOG" 2>/dev/null || true
sleep 0.3
key alt+c
ui_wait_window "Unverified Software" 20 >/dev/null && pass "file: warns that it can't be verified" || fail "file: warns that it can't be verified"
key alt+i
for _ in $(seq 1 30); do is_installed tdepk-test-local 1.0 && break; sleep 1; done
check "file: installed" is_installed tdepk-test-local 1.0
app_stop

# Authorization refused: a clear error, and nothing changes.
sudo /usr/bin/dpkg --purge tdepk-test-lib tdepk-test-app >/dev/null 2>&1
sudo /src/docker/test-services.sh polkit-rule deny
sudo /src/docker/test-services.sh restart-packagekit
app_start
search_for tdepk-test-lib
click $ROW1_BOX
press_apply
ui_wait_window "Could Not Apply Changes" 30 >/dev/null && pass "refused authorization is reported" || fail "refused authorization is reported"
[ "${TAKE_SCREENSHOTS:-0}" = 1 ] && SHOT_DIR=/src/screenshots ui_shot not-authorized
key Return
check "refused authorization changes nothing" is_absent tdepk-test-lib
app_stop
sudo /src/docker/test-services.sh polkit-rule allow

echo "UI tests against the real daemon: $FAILS failure(s)"
exit "$((FAILS > 0))"
