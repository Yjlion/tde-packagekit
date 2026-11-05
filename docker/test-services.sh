#!/bin/bash
# Runs as root inside the test container (through sudo). Starts the system bus,
# polkit and PackageKit, and points apt at the local fixture repository.
#
# usage: test-services.sh start [allow|deny] | stop-packagekit | polkit-rule allow|deny
set -euo pipefail

FIXTURE_REPO=/build/fixture-repo
RULE=/etc/polkit-1/rules.d/10-tdepk-test.rules

polkit_rule() {
    case "$1" in
        allow)
            # Test only: lets the unprivileged test user use PackageKit without an
            # authentication agent, which a headless container doesn't have.
            cat > "$RULE" <<'JS'
polkit.addRule(function(action, subject) {
    if (subject.user == "dev" && action.id.indexOf("org.freedesktop.packagekit.") == 0)
        return polkit.Result.YES;
});
JS
            ;;
        deny)
            cat > "$RULE" <<'JS'
polkit.addRule(function(action, subject) {
    if (subject.user == "dev" && action.id.indexOf("org.freedesktop.packagekit.") == 0 &&
        action.id != "org.freedesktop.packagekit.system-sources-refresh")
        return polkit.Result.NO;
});
JS
            ;;
    esac
    chmod 0644 "$RULE"
}

case "${1:-}" in
    start)
        mkdir -p /run/dbus /etc/polkit-1/rules.d
        polkit_rule "${2:-allow}"
        pgrep -x dbus-daemon >/dev/null || dbus-daemon --system --fork
        sleep 0.5
        if ! pgrep -x polkitd >/dev/null; then
            /usr/lib/polkit-1/polkitd --no-debug >/var/log/polkitd.log 2>&1 &
            disown
        fi
        sleep 1
        # The daemon is D-Bus activated in production. It is started by hand here
        # because the activation helper hands over to systemd, absent in the container.
        pkill -x packagekitd 2>/dev/null || true
        sleep 0.3
        /usr/libexec/packagekitd --timed-exit >/var/log/packagekitd.log 2>&1 &
        disown || true
        sleep 2
        ;;
    restart-packagekit)
        pkill -x packagekitd 2>/dev/null || true
        sleep 0.5
        rm -f /var/lib/PackageKit/transactions.db-journal
        /usr/libexec/packagekitd >/var/log/packagekitd.log 2>&1 &
        disown || true
        sleep 2
        ;;
    stop-packagekit)
        pkill -x packagekitd 2>/dev/null || true
        ;;
    polkit-rule)
        polkit_rule "$2"
        pkill -HUP -x polkitd 2>/dev/null || true
        # polkitd watches the rules directory; give it a moment to reload.
        sleep 2
        ;;
    use-fixture-repo)
        # Only the local fixture repository: quick, offline and deterministic.
        rm -f /etc/apt/sources.list.d/*.list /etc/apt/sources.list.d/*.sources /etc/apt/sources.list
        echo "deb [trusted=yes] file:$FIXTURE_REPO/$2 ./" > /etc/apt/sources.list.d/tdepk-fixture.list
        apt-get update -qq
        ;;
    *)
        echo "usage: $0 start|stop-packagekit|restart-packagekit|polkit-rule|use-fixture-repo" >&2
        exit 2
        ;;
esac
