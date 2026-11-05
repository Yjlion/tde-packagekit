#!/bin/bash
# Shared helpers for the UI tests. See session.sh for the X session itself.
. /src/tests/ui/session.sh

APP=/build/tree/src/app/tdepackagekit
NOTIFIER=/build/tree/src/notifier/tdepackagekit-notifier
export PATH=/build/tree/src/app:/build/tree/src/notifier:$PATH
FAILS=0

# Scribble over freed memory, so that use-after-free bugs fail reliably, and leave
# the crash handler in place: a crash in any test then shows up in app_stop.
export MALLOC_PERTURB_=165

pass() { echo "ok   - $1"; }
fail() { echo "FAIL - $1"; FAILS=$((FAILS + 1)); SHOT_DIR=/build/failures ui_shot "fail-$(date +%s%N)"; }
check() { # description command...
    local description=$1; shift
    if "$@"; then pass "$description"; else fail "$description"; fi
}

# The main window sits at a known position so that clicks can use coordinates.
app_start() {
    "$APP" --nofork "$@" >/tmp/app.log 2>&1 &
    APP_PID=$!
    WIN=$(ui_wait_window "Software Management" 30)
    xdotool windowmove --sync "$WIN" 0 0 2>/dev/null || true
    sleep 1.5
    xdotool windowfocus "$WIN" 2>/dev/null || true
}

app_stop() {
    if ! kill -0 "$APP_PID" 2>/dev/null || pgrep -x drkonqi >/dev/null; then
        fail "the application did not crash"
        pkill -x drkonqi 2>/dev/null || true
    fi
    kill "$APP_PID" 2>/dev/null || true
    wait "$APP_PID" 2>/dev/null || true
    sleep 0.5
}

click() { xdotool mousemove --sync "$1" "$2" click 1; sleep 0.3; }
key() { xdotool key --clearmodifiers "$@"; sleep 0.3; }
type_text() { xdotool type --delay 15 "$1"; sleep 0.2; }

# Positions in the 1000x680 main window, placed at the top-left of the screen.
ROW1_BOX="377 161"; ROW2_BOX="377 181"; ROW3_BOX="377 201"
ROW1_NAME="450 161"; ROW2_NAME="450 181"; ROW3_NAME="450 201"
TAB_FILES="480 442"; TAB_DEPENDS="565 442"; TAB_REQUIRED="700 442"

search_for() {
    key ctrl+1
    key ctrl+a
    type_text "$1"
    key Return
    sleep 1.5
}

window_visible() { xdotool search --onlyvisible --name "$1" >/dev/null 2>&1; }

press_apply() { key alt+a; }

# Fake daemon ----------------------------------------------------------------

fake_start() { # extra daemon arguments
    FAKE_LOG=$(mktemp)
    dbus-daemon --session --nofork --print-address 3 3>/tmp/fakebus.addr &
    FAKE_BUS_PID=$!
    for _ in $(seq 1 50); do [ -s /tmp/fakebus.addr ] && break; sleep 0.1; done
    export DBUS_SYSTEM_BUS_ADDRESS
    DBUS_SYSTEM_BUS_ADDRESS=$(head -1 /tmp/fakebus.addr)
    python3 /src/tests/fake_pk_daemon.py --log "$FAKE_LOG" "$@" >/tmp/fakepk.log 2>&1 &
    FAKE_PID=$!
    sleep 1
}

fake_stop() {
    kill "$FAKE_PID" "$FAKE_BUS_PID" 2>/dev/null || true
    wait "$FAKE_PID" "$FAKE_BUS_PID" 2>/dev/null || true
    unset DBUS_SYSTEM_BUS_ADDRESS
}

fake_called() { grep -q -- "$1" "$FAKE_LOG"; }

is_installed() { # name [version]
    local version
    version=$(dpkg-query -W -f '${db:Status-Status} ${Version}' "$1" 2>/dev/null) || return 1
    [ "${version%% *}" = installed ] || return 1
    [ -z "${2:-}" ] || [ "${version##* }" = "$2" ]
}
is_absent() { ! is_installed "$1"; }
