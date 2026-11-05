#!/bin/bash
# Builds the local apt repositories used by the real-daemon tests.
#   v1: tdepk-test-hello 1.0, tdepk-test-app 1.0 (depends on tdepk-test-lib), tdepk-test-lib 1.0
#   v2: the same, with tdepk-test-hello at 1.1 and a repo-only package removed
# Also leaves tdepk-test-local_1.0_all.deb for testing local file installation.
set -euo pipefail

OUT="${1:-/build/fixture-repo}"
rm -rf "$OUT"
mkdir -p "$OUT/v1" "$OUT/v2" "$OUT/src"

make_deb() { # name version depends destdir description
    local name=$1 version=$2 depends=$3 dest=$4 description=$5
    local tree="$OUT/src/$name-$version"
    mkdir -p "$tree/DEBIAN" "$tree/usr/share/doc/$name"
    {
        echo "Package: $name"
        echo "Version: $version"
        echo "Section: utils"
        echo "Priority: optional"
        echo "Architecture: all"
        echo "Maintainer: TDE PackageKit tests <tests@example.invalid>"
        [ -n "$depends" ] && echo "Depends: $depends"
        echo "Description: $description"
        echo " A package built for the tdepackagekit integration tests. It installs"
        echo " one small documentation file and nothing else."
    } > "$tree/DEBIAN/control"
    echo "$name $version" > "$tree/usr/share/doc/$name/README"
    dpkg-deb --root-owner-group -b "$tree" "$dest/${name}_${version}_all.deb" >/dev/null
}

make_deb tdepk-test-hello 1.0 ""               "$OUT/v1" "Friendly greeting test package"
make_deb tdepk-test-lib   1.0 ""               "$OUT/v1" "Shared library test package"
make_deb tdepk-test-app   1.0 "tdepk-test-lib" "$OUT/v1" "Application test package with a dependency"

cp "$OUT"/v1/*.deb "$OUT/v2/"
rm "$OUT/v2/tdepk-test-hello_1.0_all.deb"
make_deb tdepk-test-hello 1.1 ""               "$OUT/v2" "Friendly greeting test package"

make_deb tdepk-test-local 1.0 ""               "$OUT"    "Package installed from a local file"

for repo in v1 v2; do
    (cd "$OUT/$repo" && dpkg-scanpackages --multiversion . /dev/null 2>/dev/null > Packages && gzip -kf Packages)
    (cd "$OUT/$repo" && apt-ftparchive release . > Release 2>/dev/null || true)
done
echo "fixture repositories in $OUT"
