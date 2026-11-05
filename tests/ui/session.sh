#!/bin/bash
# Sourced by the UI tests: starts Xvfb, a window manager and the TDE session basics.
export DISPLAY="${DISPLAY:-:99}"
export HOME="${TEST_HOME:-/tmp/tdepk-home}"
export LANG=C.UTF-8
export TDEDIRS=/opt/trinity
export PATH=/opt/trinity/bin:$PATH
# A Trinity login sets these. Without them TDE can't find its menu, and indexes no applications.
export XDG_CONFIG_DIRS=/opt/trinity/etc/xdg:/etc/xdg
export XDG_DATA_DIRS=/opt/trinity/share:/usr/share
mkdir -p "$HOME"

ui_session_start() {
    Xvfb "$DISPLAY" -screen 0 1100x780x24 -nolisten tcp >/tmp/xvfb.log 2>&1 &
    XVFB_PID=$!
    for _ in $(seq 1 50); do xdpyinfo >/dev/null 2>&1 && break; sleep 0.1; done
    # The Trinity session bootstrap: DCOP, and the icon and service caches.
    dcopserver --nosid >/tmp/dcop.log 2>&1 || true
    sleep 1
    kbuildsycoca --noincremental >/tmp/sycoca.log 2>&1 || true
    twin >/tmp/twin.log 2>&1 &
    WM_PID=$!
    sleep 2
}

ui_session_stop() {
    pkill -x tdepackagekit 2>/dev/null || true
    pkill -f tdepackagekit-notifier 2>/dev/null || true
    kill "${WM_PID:-0}" 2>/dev/null || true
    dcopserver_shutdown --kill >/dev/null 2>&1 || true
    kill "${XVFB_PID:-0}" 2>/dev/null || true
}

# Waits for a window whose name matches, and prints its id.
ui_wait_window() { # name timeout-seconds
    local name=$1 limit=${2:-20} i id
    for ((i = 0; i < limit * 5; i++)); do
        id=$(xdotool search --onlyvisible --name "$name" 2>/dev/null | tail -1 || true)
        if [ -n "$id" ]; then echo "$id"; return 0; fi
        sleep 0.2
    done
    echo "timed out waiting for window: $name" >&2
    return 1
}

ui_shot() { # name
    local dir="${SHOT_DIR:-/src/screenshots}"
    mkdir -p "$dir"
    import -window root "$dir/$1.png"
}
