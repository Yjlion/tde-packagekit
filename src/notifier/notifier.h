/*
 * Tray icon that tells the user about available updates.
 * It only notifies, and never installs anything.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef TDEPK_NOTIFIER_H
#define TDEPK_NOTIFIER_H

#include <dcopobject.h>
#include <ksystemtray.h>

#include "pktypes.h"

class PkTransaction;
class TQTimer;

class Notifier : public KSystemTray, public DCOPObject
{
    TQ_OBJECT
public:
    Notifier();

    bool process(const TQCString &fun, const TQByteArray &data,
                 TQCString &replyType, TQByteArray &replyData) override;
    QCStringList functions() override;

protected:
    void contextMenuAboutToShow(TDEPopupMenu *menu) override;
    void mousePressEvent(TQMouseEvent *event) override;

private slots:
    void onTick();
    void checkNow();
    void openUpdates();
    void openSettings();
    void reloadSettings();
    void onRefreshFinished(uint exit, uint runtime);
    void onPackage(const PkPackage &package);
    void onUpdatesFinished(uint exit, uint runtime);
    void onUpdatesChanged();

private:
    void listUpdates();
    void showState();

    PkTransaction *m_refresh;
    PkTransaction *m_updates;
    TQTimer *m_timer;
    uint m_total;
    uint m_security;
    uint m_lastNotified;
    bool m_pendingList;
    bool m_known; /* an update list has been read at least once */
};

#endif
