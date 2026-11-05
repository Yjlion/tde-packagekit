/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "transactiondialog.h"

#include <tqevent.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlistview.h>
#include <tqtimer.h>
#include <tqvbox.h>

#include <kprogress.h>
#include <tdelistview.h>
#include <tdelocale.h>

#include "packageid.h"
#include "pkenums.h"
#include "pktransaction.h"

TransactionDialog::TransactionDialog(TQWidget *parent)
    : KDialogBase(parent, "transactiondialog", true, i18n("Applying Changes"), Cancel, Cancel, true),
      m_transaction(nullptr)
{
    TQVBox *page = makeVBoxMainWidget();
    m_title = new TQLabel(page, "transactiontitle");
    TQFont bold = m_title->font();
    bold.setBold(true);
    m_title->setFont(bold);
    m_status = new TQLabel(page, "transactionstatus");
    m_progress = new KProgress(page, "transactionprogress");
    m_packages = new TDEListView(page, "transactionpackages");
    m_packages->addColumn(i18n("Package"));
    m_packages->addColumn(i18n("Action"));
    m_packages->setRootIsDecorated(false);
    m_packages->setAllColumnsShowFocus(true);
    m_packages->setSorting(-1);
    m_packages->setColumnWidth(0, 320);
    m_packages->setColumnWidth(1, 140);
    m_pulse = new TQTimer(this);
    connect(m_pulse, TQ_SIGNAL(timeout()), TQ_SLOT(onPulse()));
    resize(540, 340);
}

void TransactionDialog::attach(PkTransaction *t, const TQString &title)
{
    detach();
    m_transaction = t;
    m_title->setText(title);
    m_status->setText(i18n("Starting..."));
    m_packages->clear();
    m_items.clear();
    onProgress(101);
    enableButtonCancel(false);
    connect(t, TQ_SIGNAL(statusChanged(uint)), TQ_SLOT(onStatus(uint)));
    connect(t, TQ_SIGNAL(progressChanged(uint)), TQ_SLOT(onProgress(uint)));
    connect(t, TQ_SIGNAL(allowCancelChanged(bool)), TQ_SLOT(onAllowCancel(bool)));
    connect(t, TQ_SIGNAL(package(const PkPackage &)), TQ_SLOT(onPackage(const PkPackage &)));
}

void TransactionDialog::detach()
{
    if (m_transaction) {
        m_transaction->disconnect(this);
    }
    m_transaction = nullptr;
    m_pulse->stop();
}

void TransactionDialog::onStatus(uint status)
{
    m_status->setText(Pk::statusLabel(status));
}

void TransactionDialog::onProgress(uint percentage)
{
    if (percentage > 100) {
        /* Unknown progress: a moving bar. */
        m_progress->setTotalSteps(0);
        if (!m_pulse->isActive()) {
            m_pulse->start(100);
        }
        return;
    }
    m_pulse->stop();
    m_progress->setTotalSteps(100);
    m_progress->setProgress(percentage);
}

void TransactionDialog::onPulse()
{
    m_progress->advance(1);
}

void TransactionDialog::onAllowCancel(bool allowed)
{
    enableButtonCancel(allowed);
}

void TransactionDialog::onPackage(const PkPackage &package)
{
    const PackageId id(package.id);
    if (!id.isValid()) {
        return;
    }
    TQMap<TQString, TQListViewItem *>::iterator it = m_items.find(id.name());
    TQListViewItem *item;
    if (it == m_items.end()) {
        item = new TQListViewItem(m_packages, m_packages->lastItem(), id.display(), Pk::infoLabel(package.info));
        m_items.insert(id.name(), item);
    } else {
        item = it.data();
        item->setText(1, Pk::infoLabel(package.info));
    }
    m_packages->ensureItemVisible(item);
}

void TransactionDialog::slotCancel()
{
    if (m_transaction) {
        enableButtonCancel(false);
        m_status->setText(Pk::statusLabel(Pk::StatusCancel));
        m_transaction->cancel();
    }
}

void TransactionDialog::closeEvent(TQCloseEvent *event)
{
    /* The dialog goes away when the flow ends, and closing it would hide progress. */
    event->ignore();
}

#include "transactiondialog.moc"
