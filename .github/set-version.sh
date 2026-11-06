#!/bin/bash
# Stamps a version on the Debian package: adds a debian/changelog entry for it,
# unless the newest entry already has it. The build passes the changelog's version
# on to the program (debian/rules), so this is the only place it needs to change.
# Usage: set-version.sh <version or vVERSION tag> [source dir]
set -euo pipefail

VERSION=${1#v}
DIR=${2:-.}
CHANGELOG=$DIR/debian/changelog

# Debian upstream versions start with a digit, and use only these characters.
if ! [[ $VERSION =~ ^[0-9][A-Za-z0-9.+~-]*$ ]]; then
    echo "error: '$1' isn't a version tag like v1.2.3" >&2
    exit 1
fi

current=$(sed -n '1s/^[^(]*(\([^)]*\)).*/\1/p' "$CHANGELOG")
if [ "$current" = "$VERSION" ]; then
    echo "debian/changelog is already at $VERSION"
    exit 0
fi

package=$(sed -n '1s/ .*//p' "$CHANGELOG")
maintainer=$(grep -m1 '^ -- ' "$CHANGELOG" | sed 's/^ -- \(.*>\).*/\1/')
{
    printf '%s (%s) unstable; urgency=medium\n\n' "$package" "$VERSION"
    printf '  * Release %s.\n\n' "$VERSION"
    printf ' -- %s  %s\n\n' "$maintainer" "$(LC_ALL=C date -R)"
    cat "$CHANGELOG"
} > "$CHANGELOG.new"
mv "$CHANGELOG.new" "$CHANGELOG"
echo "debian/changelog: $current -> $VERSION"
