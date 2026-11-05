#!/bin/bash
# Runs inside the container. Called by ./dk and by GitHub Actions through ./dk.
set -euo pipefail
cd /src
BUILD=/build/tree

configure() {
    cmake -S . -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_INSTALL_PREFIX=/opt/trinity -DBUILD_TESTING=ON -DWITH_GCC_VISIBILITY=OFF
}
build() { [ -f "$BUILD/CMakeCache.txt" ] || configure; cmake --build "$BUILD" -- -j"$(nproc)"; }

services() { sudo /src/docker/test-services.sh "$@"; }

case "${1:-}" in
    build) configure; build ;;
    test)
        build
        "$BUILD/tests/unit/test_core"
        python3 -W ignore::ResourceWarning tests/integration/test_core_fake.py
        tests/fixtures/build-repo.sh
        services start allow
        python3 -W ignore::ResourceWarning tests/integration/test_core_real.py
        timeout 300 tests/ui/test_ui_fake.sh
        timeout 300 tests/ui/test_ui_real.sh
        ;;
    screenshot)
        build
        TAKE_SCREENSHOTS=1 tests/ui/test_ui_fake.sh
        tests/fixtures/build-repo.sh
        services start allow
        TAKE_SCREENSHOTS=1 tests/ui/test_ui_real.sh
        ;;
    deb)
        # Builds from a copy, because dpkg-buildpackage writes next to the source.
        rm -rf /build/deb
        mkdir -p /build/deb/src
        tar -C /src --exclude=.git --exclude=dist --exclude=screenshots -cf - . | tar -C /build/deb/src -xf -
        (cd /build/deb/src && dpkg-buildpackage -us -uc -b)
        mkdir -p /src/dist
        rm -f /src/dist/*.deb
        cp /build/deb/tdepackagekit-trinity_*.deb /src/dist/
        ls -l /src/dist
        lintian --no-tag-display-limit /src/dist/tdepackagekit-trinity_*.deb || true
        ;;
    verify-deb)
        tests/ui/verify_deb.sh
        ;;
    lint)
        python3 tests/tools/check_docs.py
        desktop-file-validate data/tdepackagekit.desktop data/tdepackagekit-install-file.desktop \
            data/tdepackagekit-notifier.desktop
        bash -n dk docker/ci.sh docker/test-services.sh tests/ui/*.sh tests/fixtures/build-repo.sh
        ;;
    *) echo "unknown: ${1:-}" >&2; exit 2 ;;
esac
