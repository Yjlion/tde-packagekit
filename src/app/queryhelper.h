/*
 * Helpers shared by the pages.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef TDEPK_QUERYHELPER_H
#define TDEPK_QUERYHELPER_H

#include <tqstring.h>

class PkTransaction;
class TQObject;

namespace QueryHelper {

/*
 * Stop caring about a running transaction: its results no longer reach
 * `receiver`, it is cancelled if it can be, and it deletes itself when it ends.
 * The pointer is reset. Safe with a null pointer.
 */
void abandon(PkTransaction *&transaction, TQObject *receiver);

/* The transaction ended, so delete it and reset the pointer. */
void release(PkTransaction *&transaction);

/* Escapes text for rich-text widgets. */
TQString escapeHtml(const TQString &text);

/* A human-readable duration, such as "3 hours". */
TQString durationText(uint seconds);

} // namespace QueryHelper

#endif
