/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "updatespage.h"

#include <tqheader.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlistview.h>
#include <tqsplitter.h>
#include <tqtimer.h>

#include <kpushbutton.h>
#include <kiconloader.h>
#include <ktextbrowser.h>
#include <tdeglobal.h>
#include <tdelistview.h>
#include <tdelocale.h>

#include "changequeue.h"
#include "errordialog.h"
#include "packageicon.h"
#include "packageid.h"
#include "pkdaemon.h"
#include "pkenums.h"
#include "pktransaction.h"
#include "queryhelper.h"

enum { ColName, ColVersion, ColSummary };

class UpdateItem : public TQCheckListItem
{
public:
    UpdateItem(TQListViewItem *parent, UpdatesPage *page, const PkPackage &package)
        : TQCheckListItem(parent, PackageId(package.id).name(), TQCheckListItem::CheckBox),
          m_page(page), m_package(package)
    {
        const PackageId id(package.id);
        setPixmap(ColName, PackageIcon::small(id.name()));
        setText(ColVersion, id.version());
        setText(ColSummary, package.summary);
        setEnabled(package.info != Pk::InfoBlocked);
    }
    const PkPackage &package() const { return m_package; }
    int rtti() const override { return 2001; }

protected:
    void stateChange(bool on) override { m_page->itemToggled(this, on); }

private:
    UpdatesPage *m_page;
    PkPackage m_package;
};

static UpdateItem *asUpdate(TQListViewItem *item)
{
    return item && item->rtti() == 2001 ? static_cast<UpdateItem *>(item) : nullptr;
}

/* One heading per severity rank. */
static TQString headerLabel(uint rank)
{
    switch (rank) {
    case 0: return i18n("Critical Updates");
    case 1: return i18n("Security Updates");
    case 2: return i18n("Important Updates");
    case 3: return i18n("Bug Fixes");
    case 4: return i18n("Other Updates");
    case 5: return i18n("Enhancements");
    case 6: return i18n("Minor Updates");
    default: return i18n("Blocked Updates");
    }
}

static TQString headerIcon(uint rank)
{
    return rank <= 1 ? "messagebox_warning" : "system-software-update";
}

UpdatesPage::UpdatesPage(ChangeQueue *queue, TQWidget *parent)
    : TQWidget(parent, "updatespage"),
      m_queue(queue), m_load(nullptr), m_refreshTx(nullptr), m_detailTx(nullptr),
      m_count(0), m_errorCode(0), m_syncing(false), m_activated(false), m_gotDetail(false), m_refreshFailed(false)
{
    TQVBoxLayout *top = new TQVBoxLayout(this, 0, 6);

    TQHBoxLayout *head = new TQHBoxLayout(top, 6);
    TQVBoxLayout *labels = new TQVBoxLayout(head, 2);
    m_summary = new TQLabel(i18n("Checking for updates..."), this, "updatesummary");
    TQFont bold = m_summary->font();
    bold.setBold(true);
    m_summary->setFont(bold);
    m_lastChecked = new TQLabel(this, "lastchecked");
    labels->addWidget(m_summary);
    labels->addWidget(m_lastChecked);
    head->addStretch(1);
    m_all = new KPushButton(KGuiItem(i18n("Select A&ll"), "ok"), this, "selectall");
    m_none = new KPushButton(KGuiItem(i18n("Select &None"), "cancel"), this, "selectnone");
    m_refresh = new KPushButton(KGuiItem(i18n("&Check for Updates"), "reload"), this, "refresh");
    head->addWidget(m_all);
    head->addWidget(m_none);
    head->addWidget(m_refresh);

    TQSplitter *split = new TQSplitter(TQt::Vertical, this);
    top->addWidget(split, 1);

    m_list = new TDEListView(split, "updates");
    m_list->addColumn(i18n("Update"));
    m_list->addColumn(i18n("New Version"));
    m_list->addColumn(i18n("Summary"));
    m_list->setRootIsDecorated(true);
    m_list->setAllColumnsShowFocus(true);
    m_list->setSorting(-1);
    m_list->setColumnWidth(ColName, 230);
    m_list->setColumnWidth(ColVersion, 130);
    m_list->setColumnWidth(ColSummary, 340);

    m_detail = new KTextBrowser(split, "updatedetail");
    TQValueList<int> sizes;
    sizes << 320 << 160;
    split->setSizes(sizes);

    connect(m_all, TQ_SIGNAL(clicked()), TQ_SLOT(selectAll()));
    connect(m_none, TQ_SIGNAL(clicked()), TQ_SLOT(selectNone()));
    connect(m_refresh, TQ_SIGNAL(clicked()), TQ_SLOT(refreshCache()));
    connect(m_list, TQ_SIGNAL(selectionChanged()), TQ_SLOT(onSelectionChanged()));
    connect(m_queue, TQ_SIGNAL(changed()), TQ_SLOT(syncWithQueue()));
    connect(PkDaemon::self(), TQ_SIGNAL(timeSinceAction(uint, uint)), TQ_SLOT(onTimeSince(uint, uint)));
}

void UpdatesPage::activate()
{
    if (m_activated) {
        return;
    }
    m_activated = true;
    if (!m_load) {
        populate();
    }
}

void UpdatesPage::load()
{
    if (m_load) {
        return;
    }
    m_errorCode = 0;
    m_errorDetails = TQString();
    m_packages.clear();
    m_summary->setText(i18n("Checking for updates..."));
    m_load = new PkTransaction(this);
    connect(m_load, TQ_SIGNAL(package(const PkPackage &)), TQ_SLOT(onPackage(const PkPackage &)));
    connect(m_load, TQ_SIGNAL(errorCode(uint, const TQString &)), TQ_SLOT(onLoadError(uint, const TQString &)));
    connect(m_load, TQ_SIGNAL(finished(uint, uint)), TQ_SLOT(onLoadFinished(uint, uint)));
    m_load->getUpdates(Pk::bit(Pk::FilterNone));
    PkDaemon::self()->queryTimeSinceAction(Pk::RoleRefreshCache);
}

void UpdatesPage::onPackage(const PkPackage &package)
{
    m_packages.append(package);
}

void UpdatesPage::onLoadError(uint code, const TQString &details)
{
    m_errorCode = code;
    m_errorDetails = details;
}

void UpdatesPage::onLoadFinished(uint exit, uint)
{
    QueryHelper::release(m_load);
    m_count = 0;
    for (PkPackageList::const_iterator it = m_packages.begin(); it != m_packages.end(); ++it) {
        if ((*it).info != Pk::InfoBlocked) {
            ++m_count;
        }
    }
    populate();
    if (exit != Pk::ExitSuccess && m_errorCode) {
        m_summary->setText(Pk::errorLabel(m_errorCode));
    } else {
        updateSummary();
    }
    emit countChanged(m_count);
}

TQListViewItem *UpdatesPage::headerFor(uint rank)
{
    TQMap<uint, TQListViewItem *>::iterator it = m_headers.find(rank);
    if (it != m_headers.end()) {
        return it.data();
    }
    /* Keep headings in rank order, with sorting turned off. */
    TQListViewItem *after = nullptr;
    for (TQMap<uint, TQListViewItem *>::iterator h = m_headers.begin(); h != m_headers.end(); ++h) {
        if (h.key() < rank) {
            after = h.data();
        }
    }
    TQListViewItem *header = after ? new TQListViewItem(m_list, after, headerLabel(rank))
                                   : new TQListViewItem(m_list, headerLabel(rank));
    header->setPixmap(ColName, SmallIcon(headerIcon(rank)));
    header->setOpen(true);
    header->setSelectable(false);
    m_headers.insert(rank, header);
    return header;
}

void UpdatesPage::populate()
{
    m_syncing = true;
    /* Stale queue entries from an earlier list go away; ticks made by the user
       on packages still listed survive through the queue itself. */
    TQStringList current;
    for (PkPackageList::const_iterator it = m_packages.begin(); it != m_packages.end(); ++it) {
        current << (*it).id;
    }
    TQStringList stale;
    const TQStringList queued = m_queue->ids(ChangeQueue::Update);
    for (TQStringList::const_iterator it = queued.begin(); it != queued.end(); ++it) {
        if (current.find(*it) == current.end()) {
            stale << *it;
        }
    }

    m_list->clear();
    m_headers.clear();
    m_detail->clear();
    for (PkPackageList::const_iterator it = m_packages.begin(); it != m_packages.end(); ++it) {
        const uint rank = Pk::updateSeverityRank((*it).info);
        new UpdateItem(headerFor(rank), this, *it);
    }
    m_syncing = false;

    m_queue->setMany(stale, ChangeQueue::None);
    if (m_activated) {
        TQStringList toTick;
        const TQStringList selectable = allSelectableIds();
        for (TQStringList::const_iterator it = selectable.begin(); it != selectable.end(); ++it) {
            if (m_userUnticked.find(*it) == m_userUnticked.end()) {
                toTick << *it;
            }
        }
        m_queue->setMany(toTick, ChangeQueue::Update);
    }
    syncWithQueue();
    m_all->setEnabled(m_count > 0);
    m_none->setEnabled(m_count > 0);
}

TQStringList UpdatesPage::allSelectableIds() const
{
    TQStringList ids;
    for (TQListViewItemIterator it(m_list); it.current(); ++it) {
        UpdateItem *item = asUpdate(it.current());
        if (item && item->package().info != Pk::InfoBlocked) {
            ids << item->package().id;
        }
    }
    return ids;
}

void UpdatesPage::updateSummary()
{
    if (m_count == 0) {
        m_summary->setText(i18n("Your system is up to date."));
        return;
    }
    m_summary->setText(i18n("1 update is available.", "%n updates are available.", m_count));
}

void UpdatesPage::syncWithQueue()
{
    m_syncing = true;
    for (TQListViewItemIterator it(m_list); it.current(); ++it) {
        UpdateItem *item = asUpdate(it.current());
        if (item) {
            item->setOn(m_queue->action(item->package().id) == ChangeQueue::Update);
        }
    }
    m_syncing = false;
}

void UpdatesPage::itemToggled(UpdateItem *item, bool on)
{
    if (m_syncing) {
        return;
    }
    const TQString id = item->package().id;
    if (on) {
        m_userUnticked.remove(id);
    } else if (m_userUnticked.find(id) == m_userUnticked.end()) {
        m_userUnticked << id;
    }
    m_queue->set(id, on ? ChangeQueue::Update : ChangeQueue::None);
}

void UpdatesPage::selectAll()
{
    m_userUnticked.clear();
    m_queue->setMany(allSelectableIds(), ChangeQueue::Update);
}

void UpdatesPage::selectNone()
{
    m_userUnticked = allSelectableIds();
    m_queue->setMany(m_userUnticked, ChangeQueue::None);
}

void UpdatesPage::refreshCache()
{
    if (m_refreshTx) {
        return;
    }
    m_refresh->setEnabled(false);
    m_errorCode = 0;
    m_errorDetails = TQString();
    m_summary->setText(i18n("Refreshing package information..."));
    m_refreshTx = new PkTransaction(this);
    connect(m_refreshTx, TQ_SIGNAL(errorCode(uint, const TQString &)), TQ_SLOT(onRefreshError(uint, const TQString &)));
    connect(m_refreshTx, TQ_SIGNAL(progressChanged(uint)), TQ_SLOT(onRefreshProgress(uint)));
    connect(m_refreshTx, TQ_SIGNAL(statusChanged(uint)), TQ_SLOT(onRefreshStatus(uint)));
    connect(m_refreshTx, TQ_SIGNAL(finished(uint, uint)), TQ_SLOT(onRefreshFinished(uint, uint)));
    m_refreshTx->refreshCache(true);
}

void UpdatesPage::onRefreshError(uint code, const TQString &details)
{
    m_errorCode = code;
    m_errorDetails = details;
}

void UpdatesPage::onRefreshProgress(uint percentage)
{
    if (percentage <= 100) {
        m_summary->setText(i18n("Refreshing package information (%1%)...").arg(percentage));
    }
}

void UpdatesPage::onRefreshStatus(uint status)
{
    m_lastChecked->setText(Pk::statusLabel(status));
}

void UpdatesPage::onRefreshFinished(uint exit, uint)
{
    QueryHelper::release(m_refreshTx);
    m_refresh->setEnabled(true);
    m_lastChecked->clear();
    m_refreshFailed = exit == Pk::ExitFailed && m_errorCode;
    /* The error box can't open inside the D-Bus callback. See pktransaction.h. */
    TQTimer::singleShot(0, this, TQ_SLOT(afterRefresh()));
}

void UpdatesPage::afterRefresh()
{
    if (m_refreshFailed) {
        ErrorDialog::report(this, i18n("Could Not Refresh"), Pk::errorLabel(m_errorCode), m_errorDetails);
    }
    load();
}

void UpdatesPage::onTimeSince(uint role, uint seconds)
{
    if (role != Pk::RoleRefreshCache) {
        return;
    }
    if (seconds == TQ_UINT32(-1) || seconds == 0) {
        m_lastChecked->setText(i18n("Package information has not been refreshed yet."));
    } else {
        m_lastChecked->setText(i18n("Last refreshed %1 ago.").arg(QueryHelper::durationText(seconds)));
    }
}

void UpdatesPage::onSelectionChanged()
{
    QueryHelper::abandon(m_detailTx, this);
    m_detail->clear();
    UpdateItem *item = asUpdate(m_list->selectedItem());
    if (!item) {
        return;
    }
    m_gotDetail = false;
    m_detail->setText(i18n("Getting update details..."));
    m_detailTx = new PkTransaction(this);
    connect(m_detailTx, TQ_SIGNAL(updateDetail(const PkUpdateDetail &)), TQ_SLOT(onUpdateDetail(const PkUpdateDetail &)));
    connect(m_detailTx, TQ_SIGNAL(finished(uint, uint)), TQ_SLOT(onDetailFinished(uint, uint)));
    m_detailTx->getUpdateDetail(TQStringList(item->package().id));
}

static TQString linkList(const TQString &title, const TQStringList &urls)
{
    using QueryHelper::escapeHtml;
    if (urls.isEmpty()) {
        return TQString();
    }
    TQString html = "<p><b>" + escapeHtml(title) + "</b><br>";
    for (TQStringList::const_iterator it = urls.begin(); it != urls.end(); ++it) {
        const TQString url = escapeHtml(*it);
        if ((*it).startsWith("http://") || (*it).startsWith("https://")) {
            html += "<a href=\"" + TQString(url).replace('"', "&quot;") + "\">" + url + "</a><br>";
        } else {
            html += url + "<br>";
        }
    }
    return html + "</p>";
}

void UpdatesPage::onUpdateDetail(const PkUpdateDetail &d)
{
    using QueryHelper::escapeHtml;
    m_gotDetail = true;
    const PackageId id(d.id);
    TQString html = "<h3>" + escapeHtml(id.display()) + "</h3>";
    const TQString restart = Pk::restartLabel(d.restart);
    if (!restart.isEmpty()) {
        html += "<p><b>" + escapeHtml(restart) + "</b></p>";
    }
    if (!d.updateText.isEmpty()) {
        html += "<p>" + escapeHtml(d.updateText).replace('\n', "<br>") + "</p>";
    }
    if (!d.updates.isEmpty()) {
        TQStringList names;
        for (TQStringList::const_iterator it = d.updates.begin(); it != d.updates.end(); ++it) {
            names << PackageId(*it).display();
        }
        html += "<p><b>" + escapeHtml(i18n("Replaces:")) + "</b> " + escapeHtml(names.join(", ")) + "</p>";
    }
    html += linkList(i18n("Security advisories:"), d.cveUrls);
    html += linkList(i18n("Bug reports:"), d.bugzillaUrls);
    html += linkList(i18n("Vendor information:"), d.vendorUrls);
    if (!d.changelog.isEmpty()) {
        html += "<p><b>" + escapeHtml(i18n("Changes:")) + "</b><br>" + escapeHtml(d.changelog).replace('\n', "<br>") + "</p>";
    }
    m_detail->setText(html);
}

void UpdatesPage::onDetailFinished(uint, uint)
{
    if (!m_gotDetail) {
        m_detail->setText(i18n("No details are available for this update."));
    }
    QueryHelper::release(m_detailTx);
}

#include "updatespage.moc"
