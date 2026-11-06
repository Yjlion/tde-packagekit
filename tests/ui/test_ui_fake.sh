#!/bin/bash
# Drives the real UI against the fake daemon: the prompts and failures, plus the
# screenshots of every page and dialog.
set -u
. /src/tests/ui/ui_lib.sh
export SHOT_DIR="${SHOT_DIR:-/src/screenshots}"
SHOTS="${TAKE_SCREENSHOTS:-0}"
shot() { [ "$SHOTS" = 1 ] && ui_shot "$1"; return 0; }

# AppStream icons as apt-config-icons would leave them. Only some packages have one,
# like on a real system.
ICONS=$(mktemp -d)
mkdir -p "$ICONS/fake-main/48x48" "$ICONS/fake-main/64x64"
convert -size 64x64 xc:none -fill '#3a7bd5' -draw 'roundrectangle 4,4 59,59 10,10' \
    -fill white -draw 'rectangle 16,18 47,22 rectangle 16,30 47,34 rectangle 16,42 38,46' \
    "$ICONS/fake-main/64x64/fake-editor_org.example.Editor.png"
convert -size 48x48 radial-gradient:'#ffb84d-#e0601b' "$ICONS/fake-main/48x48/fake-browser_org.example.Browser.png"
convert -size 48x48 xc:'#5aa55a' -fill white -draw 'polygon 8,14 40,14 24,30' "$ICONS/fake-main/48x48/fake-mail_org.example.Mail.png"
export TDEPK_APPSTREAM_ICON_DIRS=$ICONS

ui_session_start
trap 'fake_stop; ui_session_stop; rm -rf "$ICONS"' EXIT
fake_start
app_start
shot browse-empty

# Browsing ---------------------------------------------------------------------
search_for fake
check "search reaches the daemon" fake_called "SearchNames .* fake"
# The editor's blue icon is drawn in the results list.
blue=$(import -window root -crop 420x420+340+140 png:- 2>/dev/null |
    convert - -fuzz 12% -fill black +opaque '#3a7bd5' -fill white -opaque '#3a7bd5' -format '%[fx:mean*w*h]' info:)
[ "${blue%.*}" -gt 20 ] 2>/dev/null && pass "package list shows AppStream icons" || fail "package list shows AppStream icons ($blue px)"
shot browse-search

click $ROW1_NAME
sleep 1
check "selecting a package loads its details" fake_called GetDetails
shot browse-details

click $TAB_FILES; sleep 0.8
check "files tab asks for files" fake_called GetFiles
shot browse-files
click $TAB_DEPENDS; sleep 0.8
check "depends tab asks for dependencies" fake_called DependsOn
shot browse-depends
click $TAB_REQUIRED; sleep 0.8
check "required-by tab asks for dependents" fake_called RequiredBy

# Dependencies: confirm dialog --------------------------------------------------
search_for fake-deps
click $ROW1_BOX
shot queue-pending
press_apply
W=$(ui_wait_window "Confirm Changes" 10) && pass "extra changes need confirmation" || fail "extra changes need confirmation"
check "changes are simulated first" fake_called "InstallPackages 4 fake-deps"
shot confirm-changes
key Escape
sleep 1
if fake_called "InstallPackages 2 fake-deps"; then fail "cancelling the confirmation installs nothing"; else pass "cancelling the confirmation installs nothing"; fi

press_apply
ui_wait_window "Confirm Changes" 10 >/dev/null
key alt+c
sleep 2
check "confirming runs the real install with ONLY_TRUSTED" fake_called "InstallPackages 2 fake-deps"

# License agreement --------------------------------------------------------------
search_for fake-eula
click $ROW1_BOX
press_apply
ui_wait_window "License Agreement" 10 >/dev/null && pass "license prompt appears" || fail "license prompt appears"
shot license-agreement
# TDE's accelerator manager assigns these: I accept, Continue (Cancel takes Alt+A).
if fake_called AcceptEula; then fail "license is not accepted before asking"; fi
key alt+c
sleep 0.5
if window_visible "License Agreement"; then pass "Continue stays disabled until the box is ticked"; else fail "Continue stays disabled until the box is ticked"; fi
key alt+i
sleep 0.3
key alt+c
sleep 2
check "license is accepted through the daemon" fake_called "AcceptEula fake-eula-1"
check "install is retried after accepting" test "$(grep -c 'InstallPackages 2 fake-eula' "$FAKE_LOG")" = 2

# Signature ---------------------------------------------------------------------
search_for fake-key
click $ROW1_BOX
press_apply
ui_wait_window "Trust This Software Source" 10 >/dev/null && pass "signature prompt appears" || fail "signature prompt appears"
shot signature-prompt
key Return
sleep 1
if fake_called InstallSignature; then fail "the default button does not trust a key"; else pass "the default button does not trust a key"; fi
sleep 1
press_apply
ui_wait_window "Trust This Software Source" 10 >/dev/null
key alt+t
sleep 2
check "trusting the key installs the signature" fake_called "InstallSignature 1 ABCD1234"
check "install is retried after trusting" test "$(grep -c 'InstallPackages 2 fake-key' "$FAKE_LOG")" -ge 2

# Restart ------------------------------------------------------------------------
search_for fake-restart
click $ROW1_BOX
press_apply
ui_wait_window "Restart Required" 10 >/dev/null && pass "restart prompt appears" || fail "restart prompt appears"
shot restart-required
key Return
sleep 1

# Unverified software ------------------------------------------------------------
search_for fake-untrust
click $ROW1_BOX
press_apply
ui_wait_window "Unverified Software" 10 >/dev/null && pass "unverified prompt appears" || fail "unverified prompt appears"
shot unverified-software
key alt+i
sleep 2
check "installing anyway drops ONLY_TRUSTED" fake_called "InstallPackages 0 fake-untrust"

# Errors ---------------------------------------------------------------------------
search_for fake-fail
click $ROW1_BOX
press_apply
ERR=$(ui_wait_window "Could Not Apply Changes" 10) && pass "failures are reported" || fail "failures are reported"
shot error-dialog
# The details must be readable: the window has to grow when the user drags it.
if [ -n "${ERR:-}" ]; then
    xdotool windowsize --sync "$ERR" 900 600 2>/dev/null || true
    sleep 0.5
    size=$(xwininfo -id "$ERR" | awk '/Width:/ {w=$2} /Height:/ {h=$2} END {print w "x" h}')
    [ "$size" = 900x600 ] && pass "error dialog is resizable" || fail "error dialog is resizable (got $size)"
fi
key Return
sleep 1
key alt+r
sleep 0.5

# Progress and cancel ---------------------------------------------------------------
search_for fake-slow
click $ROW1_BOX
press_apply
ui_wait_window "Applying Changes" 10 >/dev/null && pass "progress dialog appears" || fail "progress dialog appears"
sleep 2
shot transaction-progress
key alt+c
sleep 1.5
check "cancel reaches the daemon" fake_called "^Cancel"
if window_visible "Applying Changes"; then fail "dialog closes after cancelling"; else pass "dialog closes after cancelling"; fi
key alt+r
sleep 0.5

# Removal always asks ---------------------------------------------------------------
search_for fake-browser
click $ROW1_BOX
press_apply
ui_wait_window "Confirm Changes" 10 >/dev/null && pass "removal asks first" || fail "removal asks first"
key Escape
sleep 1
key alt+r
sleep 0.5

# Updates ------------------------------------------------------------------------------
key ctrl+2
sleep 1.5
check "updates are listed" fake_called GetUpdates
shot updates
click 300 215
sleep 1
check "selecting an update loads its details" fake_called GetUpdateDetail
shot updates-details
press_apply
sleep 1
ui_wait_window "Confirm Changes" 3 >/dev/null 2>&1 && key alt+c
sleep 2.5
check "selected updates are applied" fake_called "UpdatePackages 2 "
if grep "^UpdatePackages 2" "$FAKE_LOG" | grep -q fake-driver; then fail "blocked updates are left out"; else pass "blocked updates are left out"; fi

# History -------------------------------------------------------------------------------
key ctrl+3
sleep 1
check "history is requested" fake_called GetOldTransactions
click 500 113
sleep 0.5
shot history

# Repositories ---------------------------------------------------------------------------
key ctrl+4
sleep 1
check "repositories are requested" fake_called GetRepoList
shot repositories
# Rows sort by description: backports, main, security.
click 201 140
sleep 1.5
check "ticking a repository enables it" fake_called "RepoEnable fake-backports True"
click 201 180
ui_wait_window "Could Not Change Repository" 8 >/dev/null && pass "a refused change is reported" || fail "a refused change is reported"
key Return
sleep 1

# Settings --------------------------------------------------------------------------------
"$APP" --settings >/dev/null 2>&1
ui_wait_window "Configure Software Updates" 5 >/dev/null && pass "settings open" || fail "settings open"
shot settings
key Escape
sleep 0.5

# Installing a file through the running instance ---------------------------------------------
"$APP" --install-file /tmp/some-package.deb >/dev/null 2>&1
ui_wait_window "Confirm Changes" 10 >/dev/null && pass "a package file asks for confirmation" || fail "a package file asks for confirmation"
check "the file path is made absolute and simulated" fake_called "InstallFiles 4 /tmp/some-package.deb"
shot install-file
key Escape
sleep 1

app_stop

# Notifier ---------------------------------------------------------------------------------------
cat > "$HOME/.trinity/share/config/tdepackagekitrc" 2>/dev/null <<CONFIG
[Notifier]
InitialDelaySeconds=1
LastCheck=0
CheckIntervalHours=24
Enabled=true
CONFIG
: > "$FAKE_LOG"
"$NOTIFIER" >/tmp/notifier.log 2>&1
sleep 4
check "notifier refreshes when a check is due" fake_called RefreshCache
check "notifier lists updates" fake_called GetUpdates
if grep -qE "^(Install|Update|Remove)Packages" "$FAKE_LOG"; then fail "notifier never installs anything"; else pass "notifier never installs anything"; fi
if pgrep -f tdepackagekit-notifier >/dev/null; then pass "notifier stays running"; else fail "notifier stays running"; fi
shot notifier-popup
pkill -f tdepackagekit-notifier || true

echo "UI tests against the fake daemon: $FAILS failure(s)"
exit "$((FAILS > 0))"
