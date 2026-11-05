/*
 * Value types carried by PackageKit signals.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef TDEPK_PKTYPES_H
#define TDEPK_PKTYPES_H

#include <tqstring.h>
#include <tqstringlist.h>
#include <tqvaluelist.h>

struct PkPackage {
    uint info;
    TQString id;
    TQString summary;
    PkPackage() : info(0) {}
    PkPackage(uint i, const TQString &p, const TQString &s) : info(i), id(p), summary(s) {}
};
typedef TQValueList<PkPackage> PkPackageList;

struct PkDetails {
    TQString id;
    uint group;
    TQString summary;
    TQString description;
    TQString url;
    TQString license;
    TQ_UINT64 size;
    PkDetails() : group(0), size(0) {}
};

struct PkUpdateDetail {
    TQString id;
    TQStringList updates;
    TQStringList obsoletes;
    TQStringList vendorUrls;
    TQStringList bugzillaUrls;
    TQStringList cveUrls;
    uint restart;
    TQString updateText;
    TQString changelog;
    uint state;
    TQString issued;
    TQString updated;
    PkUpdateDetail() : restart(0), state(0) {}
};

struct PkRepo {
    TQString id;
    TQString description;
    bool enabled;
    PkRepo() : enabled(false) {}
};

struct PkHistoryEntry {
    TQString path;
    TQString timespec;
    bool succeeded;
    uint role;
    uint duration;
    TQString data;
    uint uid;
    TQString cmdline;
    PkHistoryEntry() : succeeded(false), role(0), duration(0), uid(0) {}
};

struct PkSignature {
    TQString packageId;
    TQString repository;
    TQString keyUrl;
    TQString keyUserId;
    TQString keyId;
    TQString keyFingerprint;
    TQString keyTimestamp;
    uint type;
    PkSignature() : type(0) {}
};

struct PkEula {
    TQString id;
    TQString packageId;
    TQString vendor;
    TQString agreement;
};

#endif
