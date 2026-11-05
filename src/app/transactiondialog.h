/*
 * Shows the progress of the running package operation.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef TDEPK_TRANSACTIONDIALOG_H
#define TDEPK_TRANSACTIONDIALOG_H

#include <tqmap.h>

#include <kdialogbase.h>

#include "pktypes.h"

class KProgress;
class PkTransaction;
class TDEListView;
class TQLabel;
class TQListViewItem;
class TQTimer;

class TransactionDialog : public KDialogBase
{
    TQ_OBJECT
public:
    explicit TransactionDialog(TQWidget *parent = nullptr);

    /* Follows the transaction until it is replaced. It is not owned. */
    void attach(PkTransaction *transaction, const TQString &title);
    void detach();

protected:
    void slotCancel() override;
    void closeEvent(TQCloseEvent *event) override;

private slots:
    void onStatus(uint status);
    void onProgress(uint percentage);
    void onAllowCancel(bool allowed);
    void onPackage(const PkPackage &package);
    void onPulse();

private:
    TQLabel *m_title;
    TQLabel *m_status;
    KProgress *m_progress;
    TDEListView *m_packages;
    TQTimer *m_pulse;
    PkTransaction *m_transaction;
    TQMap<TQString, TQListViewItem *> m_items;
};

#endif
