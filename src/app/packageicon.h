/*
 * Small icons for package lists, from AppStream (see appstreamicons.h).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef TDEPK_PACKAGEICON_H
#define TDEPK_PACKAGEICON_H

#include <tqpixmap.h>
#include <tqstring.h>

namespace PackageIcon
{
/* The package's application icon at list size. A package without one gets a
   generic package icon so names line up, but only if the system has AppStream
   icons at all; otherwise this is a null pixmap and lists look as they did. */
TQPixmap small(const TQString &packageName);
}

#endif
