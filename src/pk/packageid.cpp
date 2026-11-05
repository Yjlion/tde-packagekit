/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "packageid.h"

#include <tqstringlist.h>

PackageId::PackageId(const TQString &id)
{
    const TQStringList parts = TQStringList::split(';', id, true);
    if (parts.count() != 4 || parts[0].isEmpty()) {
        return;
    }
    m_id = id;
    m_name = parts[0];
    m_version = parts[1];
    m_arch = parts[2];
    m_data = parts[3];
}

TQString PackageId::origin() const
{
    TQString origin = m_data;
    if (origin.startsWith("+")) {
        origin = origin.mid(1);
    }
    const int colon = origin.find(':');
    if (colon >= 0) {
        origin = origin.mid(colon + 1);
    }
    return origin == "installed" ? TQString() : origin;
}

TQString PackageId::display() const
{
    TQString text = m_name;
    if (!m_version.isEmpty()) {
        text += " " + m_version;
    }
    if (!m_arch.isEmpty()) {
        text += " (" + m_arch + ")";
    }
    return text;
}
