/*
 * PackageKit package ID: "name;version;arch;data".
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef TDEPK_PACKAGEID_H
#define TDEPK_PACKAGEID_H

#include <tqstring.h>

class PackageId
{
public:
    PackageId() {}
    explicit PackageId(const TQString &id);

    bool isValid() const { return !m_name.isEmpty(); }
    const TQString &id() const { return m_id; }
    const TQString &name() const { return m_name; }
    const TQString &version() const { return m_version; }
    const TQString &arch() const { return m_arch; }
    /* Backend data. For apt: "installed", "manual:<origin>", "auto:<origin>" or "<origin>". */
    const TQString &data() const { return m_data; }
    /* Repository the package comes from, with any installed prefix removed. */
    TQString origin() const;
    /* "name version (arch)", for showing to the user. */
    TQString display() const;

private:
    TQString m_id, m_name, m_version, m_arch, m_data;
};

#endif
