/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "packageicon.h"

#include <tqimage.h>
#include <tqmap.h>

#include <kiconloader.h>

#include "appstreamicons.h"

TQPixmap PackageIcon::small(const TQString &packageName)
{
    if (!AppStreamIcons::available()) {
        return TQPixmap();
    }
    static TQMap<TQString, TQPixmap> cache;
    TQMap<TQString, TQPixmap>::ConstIterator hit = cache.find(packageName);
    if (hit != cache.end()) {
        return *hit;
    }

    const int size = IconSize(TDEIcon::Small);
    TQPixmap pixmap;
    const TQString file = AppStreamIcons::path(packageName, size);
    TQImage image;
    if (!file.isEmpty() && image.load(file)) {
        if (image.width() != size || image.height() != size) {
            image = image.smoothScale(size, size, TQImage::ScaleMin);
        }
        pixmap.convertFromImage(image);
    }
    if (pixmap.isNull()) {
        static const TQPixmap fallback = SmallIcon("application-x-deb");
        pixmap = fallback;
    }
    cache.insert(packageName, pixmap);
    return pixmap;
}
