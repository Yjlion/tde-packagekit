#!/bin/bash
# Builds the Debian package in a Debian 13 chroot with Trinity's repository, on an
# Ubuntu GitHub runner. Run as root from the top of the source tree. The .deb goes to dist/.
set -euo pipefail

CHROOT=${CHROOT:-/srv/trixie}
DEBIAN_MIRROR=http://deb.debian.org/debian
TDE_MIRROR=http://mirror.ppa.trinitydesktop.org/trinity/deb
# Trinity's mirror redirects to plain HTTP. This key anchors apt's trust in the
# repository, so its hash is pinned. Keep it the same as docker/Dockerfile.
TDE_KEYRING_SHA256=ed7b8b5378a769dd66e554e5ffa9e750f548b93be02e6d23dbc3824462e75cd2

export DEBIAN_FRONTEND=noninteractive LC_ALL=C.UTF-8
SOURCE=$(pwd)

apt-get update -qq
apt-get install -y -qq --no-install-recommends debootstrap debian-archive-keyring curl ca-certificates

debootstrap --variant=buildd --keyring=/usr/share/keyrings/debian-archive-keyring.gpg \
    trixie "$CHROOT" "$DEBIAN_MIRROR"

cleanup() { umount "$CHROOT/proc" 2>/dev/null || true; }
trap cleanup EXIT
mount -t proc proc "$CHROOT/proc"
cp /etc/resolv.conf "$CHROOT/etc/resolv.conf"

curl -fsSL "$TDE_MIRROR/trinity-keyring.deb" -o "$CHROOT/tmp/trinity-keyring.deb"
echo "$TDE_KEYRING_SHA256  $CHROOT/tmp/trinity-keyring.deb" | sha256sum -c -

mkdir -p "$CHROOT/build/src"
tar -C "$SOURCE" --exclude=.git --exclude=dist -cf - . | tar -C "$CHROOT/build/src" -xf -
# For a release, the version comes from the tag (DEB_VERSION=v1.2.3).
if [ -n "${DEB_VERSION:-}" ]; then
    "$SOURCE/.github/set-version.sh" "$DEB_VERSION" "$CHROOT/build/src"
fi

chroot "$CHROOT" /bin/bash -euo pipefail -c "
    export DEBIAN_FRONTEND=noninteractive LC_ALL=C.UTF-8
    dpkg -i /tmp/trinity-keyring.deb
    echo 'deb $TDE_MIRROR/trinity-r14.1.x trixie main deps' > /etc/apt/sources.list.d/tde.list
    apt-get update -qq
    # debian/control is the only list of what the build needs.
    apt-get build-dep -y --no-install-recommends /build/src
    cd /build/src
    dpkg-buildpackage -us -uc -b
"

mkdir -p "$SOURCE/dist"
cp "$CHROOT"/build/tde-packagekit-trinity_*.deb "$SOURCE/dist/"
dpkg-deb --info "$SOURCE"/dist/tde-packagekit-trinity_*.deb
dpkg-deb --contents "$SOURCE"/dist/tde-packagekit-trinity_*.deb | grep -E 'bin/tdepackagekit'
