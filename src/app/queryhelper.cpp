/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "queryhelper.h"

#include <tqobject.h>
#include <tqstylesheet.h>

#include <tdelocale.h>

#include "pktransaction.h"

namespace QueryHelper {

void abandon(PkTransaction *&transaction, TQObject *receiver)
{
    if (!transaction) {
        return;
    }
    PkTransaction *t = transaction;
    transaction = nullptr;
    t->disconnect(receiver);
    if (t->isFinished()) {
        t->deleteLater();
        return;
    }
    TQObject::connect(t, TQ_SIGNAL(finished(uint, uint)), t, TQ_SLOT(deleteLater()));
    t->cancel();
}

void release(PkTransaction *&transaction)
{
    if (transaction) {
        transaction->deleteLater();
        transaction = nullptr;
    }
}

TQString escapeHtml(const TQString &text)
{
    return TQStyleSheet::escape(text);
}

TQString durationText(uint seconds)
{
    const uint minutes = seconds / 60;
    const uint hours = minutes / 60;
    const uint days = hours / 24;
    if (days > 0) {
        return i18n("1 day", "%n days", days);
    }
    if (hours > 0) {
        return i18n("1 hour", "%n hours", hours);
    }
    if (minutes > 0) {
        return i18n("1 minute", "%n minutes", minutes);
    }
    return i18n("less than a minute");
}

} // namespace QueryHelper
