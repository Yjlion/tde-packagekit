/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "appstreamicons.h"

#include <stdlib.h>

#include <tqdir.h>
#include <tqmap.h>

/* Package name -> (pixel size -> file). */
typedef TQMap<int, TQString> SizeMap;
typedef TQMap<TQString, SizeMap> IconIndex;

static IconIndex *s_index = nullptr;

static void scanSizeDir(IconIndex &index, const TQString &dir, int size)
{
    const TQStringList files = TQDir(dir).entryList("*.png", TQDir::Files);
    for (TQStringList::ConstIterator f = files.begin(); f != files.end(); ++f) {
        const int sep = (*f).find('_');
        if (sep <= 0) {
            continue;
        }
        SizeMap &sizes = index[(*f).left(sep)];
        /* A package with several applications: the first one found wins. */
        if (!sizes.contains(size)) {
            sizes[size] = dir + "/" + *f;
        }
    }
}

static IconIndex &iconIndex()
{
    if (s_index) {
        return *s_index;
    }
    s_index = new IconIndex;
    const TQStringList roots = AppStreamIcons::roots();
    for (TQStringList::ConstIterator root = roots.begin(); root != roots.end(); ++root) {
        const TQStringList origins = TQDir(*root).entryList(TQDir::Dirs | TQDir::NoSymLinks);
        for (TQStringList::ConstIterator origin = origins.begin(); origin != origins.end(); ++origin) {
            if (*origin == "." || *origin == "..") {
                continue;
            }
            const TQString originDir = *root + "/" + *origin;
            const TQStringList sizes = TQDir(originDir).entryList(TQDir::Dirs);
            for (TQStringList::ConstIterator s = sizes.begin(); s != sizes.end(); ++s) {
                const int size = AppStreamIcons::directorySize(*s);
                if (size > 0) {
                    scanSizeDir(*s_index, originDir + "/" + *s, size);
                }
            }
        }
    }
    return *s_index;
}

TQStringList AppStreamIcons::roots()
{
    const char *env = getenv("TDEPK_APPSTREAM_ICON_DIRS");
    if (env) {
        return TQStringList::split(':', TQString::fromLocal8Bit(env));
    }
    /* AppStream 1.x (Debian 13) first, then 0.x. */
    return TQStringList() << "/var/lib/swcatalog/icons" << "/var/lib/app-info/icons"
                          << "/usr/share/swcatalog/icons" << "/usr/share/app-info/icons";
}

int AppStreamIcons::directorySize(const TQString &name)
{
    TQString dims = name;
    int scale = 1;
    const int at = dims.find('@');
    if (at >= 0) {
        bool ok = false;
        scale = dims.mid(at + 1).toInt(&ok);
        if (!ok || scale < 1) {
            return 0;
        }
        dims = dims.left(at);
    }
    const int x = dims.find('x');
    if (x <= 0) {
        return 0;
    }
    bool okW = false, okH = false;
    const int w = dims.left(x).toInt(&okW);
    dims.mid(x + 1).toInt(&okH);
    return okW && okH && w > 0 ? w * scale : 0;
}

TQString AppStreamIcons::path(const TQString &packageName, int size)
{
    IconIndex &index = iconIndex();
    IconIndex::ConstIterator it = index.find(packageName);
    if (it == index.end() || (*it).isEmpty()) {
        return TQString::null;
    }
    /* TQMap is sorted, so the first size that is big enough is the smallest one. */
    TQString largest;
    for (SizeMap::ConstIterator s = (*it).begin(); s != (*it).end(); ++s) {
        if (s.key() >= size) {
            return s.data();
        }
        largest = s.data();
    }
    return largest;
}

bool AppStreamIcons::available()
{
    return !iconIndex().isEmpty();
}

void AppStreamIcons::reset()
{
    delete s_index;
    s_index = nullptr;
}
