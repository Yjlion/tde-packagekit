/*
 * Application icons from AppStream (DEP-11) metadata.
 *
 * With apt-config-icons installed, apt downloads each repository's icon
 * tarballs and AppStream unpacks them as <root>/<origin>/<W>x<H>[@2]/<package>_<icon>.png.
 * Debian package names can't contain "_", so the file name alone tells which
 * package an icon belongs to.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef TDEPK_APPSTREAMICONS_H
#define TDEPK_APPSTREAMICONS_H

#include <tqstring.h>
#include <tqstringlist.h>

class AppStreamIcons
{
public:
    /* Icon file for a package, at least `size` pixels if there is one that big,
       otherwise the largest. Empty if the package has no icon. */
    static TQString path(const TQString &packageName, int size);

    /* True if any icons are installed at all. */
    static bool available();

    /* Where icons are looked for. TDEPK_APPSTREAM_ICON_DIRS (colon-separated)
       replaces the system list, for tests. */
    static TQStringList roots();

    /* Forgets the index, so the next call scans again. */
    static void reset();

    /* Pixel size a cache directory name stands for: "64x64" is 64, "64x64@2" is 128.
       0 if the name isn't a size. */
    static int directorySize(const TQString &name);
};

#endif
