/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "historypage.h"

#include <tqdatetime.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlistview.h>
#include <tqsplitter.h>

#include <ktextbrowser.h>
#include <tdeglobal.h>
#include <tdelistview.h>
#include <tdelocale.h>

#include "packageid.h"
#include "pkenums.h"
#include "pktransaction.h"
#include "queryhelper.h"

enum { ColDate, ColAction, ColPackages, ColResult, ColSortKey, ColData };

static const uint HistoryLimit = 200;

static bool roleChangesSystem(uint role)
{
    switch (role) {
    case Pk::RoleInstallFiles:
    case Pk::RoleInstallPackages:
    case Pk::RoleRemovePackages:
    case Pk::RoleUpdatePackages:
    case Pk::RoleRepoEnable:
    case Pk::RoleInstallSignature:
    case Pk::RoleUpgradeSystem:
    case Pk::RoleRepairSystem:
        return true;
    default:
        return false;
    }
}

/* The Transaction signal's data holds lines of "info\tpackage-id". */
static TQStringList packageNames(const TQString &data)
{
    TQStringList names;
    const TQStringList lines = TQStringList::split('\n', data);
    for (TQStringList::const_iterator it = lines.begin(); it != lines.end(); ++it) {
        const int tab = (*it).find('\t');
        if (tab < 0) {
            continue;
        }
        const PackageId id((*it).mid(tab + 1));
        if (id.isValid() && names.find(id.name()) == names.end()) {
            names << id.name();
        }
    }
    return names;
}

static TQDateTime parseTime(const TQString &timespec)
{
    /* ISO 8601, for example 2026-11-05T13:07:26Z or with fractional seconds. */
    TQString text = timespec;
    const int dot = text.find('.');
    if (dot > 0) {
        text.truncate(dot);
    }
    if (text.endsWith("Z")) {
        text.truncate(text.length() - 1);
    }
    TQDateTime utc = TQDateTime::fromString(text, TQt::ISODate);
    if (!utc.isValid()) {
        return utc;
    }
    /* Convert from UTC to local time. */
    const TQDateTime now = TQDateTime::currentDateTime();
    const TQDateTime nowUtc = TQDateTime::currentDateTime(TQt::UTC);
    return utc.addSecs(nowUtc.secsTo(now));
}

HistoryPage::HistoryPage(TQWidget *parent)
    : TQWidget(parent, "historypage"), m_query(nullptr), m_errorCode(0), m_shown(0)
{
    TQVBoxLayout *top = new TQVBoxLayout(this, 0, 6);
    m_status = new TQLabel(this, "historystatus");
    top->addWidget(m_status);

    TQSplitter *split = new TQSplitter(TQt::Vertical, this);
    top->addWidget(split, 1);

    m_list = new TDEListView(split, "history");
    m_list->addColumn(i18n("Date"));
    m_list->addColumn(i18n("Action"));
    m_list->addColumn(i18n("Packages"));
    m_list->addColumn(i18n("Result"));
    m_list->setAllColumnsShowFocus(true);
    m_list->setRootIsDecorated(false);
    m_list->setColumnWidth(ColDate, 170);
    m_list->setColumnWidth(ColAction, 180);
    m_list->setColumnWidth(ColPackages, 300);
    m_list->setColumnWidth(ColResult, 90);
    m_list->setSorting(-1);

    m_detail = new KTextBrowser(split, "historydetail");
    TQValueList<int> sizes;
    sizes << 340 << 140;
    split->setSizes(sizes);

    connect(m_list, TQ_SIGNAL(selectionChanged()), TQ_SLOT(onSelectionChanged()));
}

void HistoryPage::load()
{
    QueryHelper::abandon(m_query, this);
    m_list->clear();
    m_detail->clear();
    m_errorCode = 0;
    m_shown = 0;
    m_status->setText(i18n("Loading history..."));
    m_query = new PkTransaction(this);
    connect(m_query, TQ_SIGNAL(historyEntry(const PkHistoryEntry &)), TQ_SLOT(onEntry(const PkHistoryEntry &)));
    connect(m_query, TQ_SIGNAL(errorCode(uint, const TQString &)), TQ_SLOT(onError(uint, const TQString &)));
    connect(m_query, TQ_SIGNAL(finished(uint, uint)), TQ_SLOT(onFinished(uint, uint)));
    m_query->getOldTransactions(HistoryLimit);
}

void HistoryPage::onEntry(const PkHistoryEntry &e)
{
    if (!roleChangesSystem(e.role)) {
        return;
    }
    const TQDateTime when = parseTime(e.timespec);
    const TQString date = when.isValid() ? TDEGlobal::locale()->formatDateTime(when, true) : e.timespec;
    TQListViewItem *item = new TQListViewItem(m_list, date, Pk::roleLabelPast(e.role),
                                              packageNames(e.data).join(", "),
                                              e.succeeded ? i18n("Succeeded") : i18n("Failed"));
    item->setText(ColData, e.data);
    ++m_shown;
}

void HistoryPage::onError(uint code, const TQString &)
{
    m_errorCode = code;
}

void HistoryPage::onFinished(uint exit, uint)
{
    QueryHelper::release(m_query);
    if (exit != Pk::ExitSuccess && m_errorCode) {
        m_status->setText(Pk::errorLabel(m_errorCode));
    } else if (m_shown == 0) {
        m_status->setText(i18n("No software changes have been recorded yet."));
    } else {
        m_status->setText(i18n("1 change recorded.", "%n changes recorded.", m_shown));
    }
}

void HistoryPage::onSelectionChanged()
{
    using QueryHelper::escapeHtml;
    TQListViewItem *item = m_list->selectedItem();
    if (!item) {
        m_detail->clear();
        return;
    }
    TQString html = "<b>" + escapeHtml(item->text(ColAction)) + "</b> &mdash; " + escapeHtml(item->text(ColDate)) + "<br>";
    const TQStringList lines = TQStringList::split('\n', item->text(ColData));
    for (TQStringList::const_iterator it = lines.begin(); it != lines.end(); ++it) {
        const int tab = (*it).find('\t');
        const PackageId id(tab >= 0 ? (*it).mid(tab + 1) : *it);
        if (id.isValid()) {
            const TQString action = tab > 0 ? (*it).left(tab) : TQString();
            html += escapeHtml(id.display());
            if (!action.isEmpty()) {
                html += " &mdash; " + escapeHtml(action);
            }
            html += "<br>";
        }
    }
    m_detail->setText(html);
}

#include "historypage.moc"
