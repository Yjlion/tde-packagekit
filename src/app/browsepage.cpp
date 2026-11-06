/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "browsepage.h"

#include <tqcheckbox.h>
#include <tqheader.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlistview.h>
#include <tqsplitter.h>
#include <tqtimer.h>

#include <kcombobox.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>
#include <ktabwidget.h>
#include <ktextbrowser.h>
#include <tdeglobal.h>
#include <tdeio/global.h>
#include <tdelistview.h>
#include <tdelocale.h>

#include "changequeue.h"
#include "packageicon.h"
#include "packageid.h"
#include "pkdaemon.h"
#include "pkenums.h"
#include "pktransaction.h"
#include "queryhelper.h"

enum { ColName, ColVersion, ColStatus, ColSummary };
enum { ModeName, ModeDescription, ModeFile };
enum { StateAll, StateInstalled, StateAvailable };

class PackageItem : public TQCheckListItem
{
public:
    PackageItem(TQListView *list, BrowsePage *page, const PkPackage &package)
        : TQCheckListItem(list, PackageId(package.id).name(), TQCheckListItem::CheckBox),
          m_page(page), m_package(package)
    {
        setPackage(package);
    }

    void setPackage(const PkPackage &package)
    {
        m_package = package;
        const PackageId id(package.id);
        setText(ColName, id.name());
        setPixmap(ColName, PackageIcon::small(id.name()));
        setText(ColVersion, id.version());
        setText(ColSummary, package.summary);
    }

    const PkPackage &package() const { return m_package; }
    bool installed() const { return Pk::infoIsInstalled(m_package.info); }

protected:
    void stateChange(bool on) override { m_page->itemToggled(this, on); }

private:
    BrowsePage *m_page;
    PkPackage m_package;
};

static TQString itemKey(const PkPackage &package)
{
    const PackageId id(package.id);
    return id.name() + ";" + id.arch();
}

BrowsePage::BrowsePage(ChangeQueue *queue, TQWidget *parent)
    : TQWidget(parent, "browsepage"),
      m_queue(queue), m_query(nullptr), m_info(nullptr),
      m_errorCode(0), m_found(0), m_syncing(false),
      m_lastKind(NoQuery), m_lastMode(ModeName), m_infoPage(-1), m_infoList(nullptr)
{
    TQVBoxLayout *top = new TQVBoxLayout(this, 0, 6);

    TQHBoxLayout *searchRow = new TQHBoxLayout(top, 6);
    m_search = new KLineEdit(this, "search");
    m_search->setClickMessage(i18n("Search for packages"));
    m_mode = new KComboBox(this, "searchmode");
    m_mode->insertItem(i18n("By Name"), ModeName);
    m_mode->insertItem(i18n("By Description"), ModeDescription);
    m_mode->insertItem(i18n("By File Name"), ModeFile);
    m_find = new KPushButton(KGuiItem(i18n("&Find"), "edit-find"), this, "find");
    searchRow->addWidget(m_search, 1);
    searchRow->addWidget(m_mode);
    searchRow->addWidget(m_find);

    TQHBoxLayout *filterRow = new TQHBoxLayout(top, 6);
    filterRow->addWidget(new TQLabel(i18n("Show:"), this));
    m_stateFilter = new KComboBox(this, "statefilter");
    m_stateFilter->insertItem(i18n("All Packages"), StateAll);
    m_stateFilter->insertItem(i18n("Installed Only"), StateInstalled);
    m_stateFilter->insertItem(i18n("Available Only"), StateAvailable);
    filterRow->addWidget(m_stateFilter);
    m_guiOnly = new TQCheckBox(i18n("&Graphical applications only"), this, "guionly");
    m_newestOnly = new TQCheckBox(i18n("&Newest versions only"), this, "newestonly");
    m_newestOnly->setChecked(true);
    filterRow->addWidget(m_guiOnly);
    filterRow->addWidget(m_newestOnly);
    filterRow->addStretch(1);

    TQSplitter *hsplit = new TQSplitter(TQt::Horizontal, this);
    top->addWidget(hsplit, 1);

    m_groups = new TDEListView(hsplit, "groups");
    m_groups->addColumn(i18n("Categories"));
    m_groups->setResizeMode(TQListView::LastColumn);
    m_groups->setRootIsDecorated(false);
    m_groups->setShowSortIndicator(false);

    TQSplitter *vsplit = new TQSplitter(TQt::Vertical, hsplit);

    m_results = new TDEListView(vsplit, "results");
    m_results->addColumn(i18n("Package"));
    m_results->addColumn(i18n("Version"));
    m_results->addColumn(i18n("Status"));
    m_results->addColumn(i18n("Summary"));
    m_results->setAllColumnsShowFocus(true);
    m_results->setRootIsDecorated(false);
    m_results->setResizeMode(TQListView::LastColumn);
    m_results->setColumnWidth(ColName, 190);
    m_results->setColumnWidth(ColVersion, 90);
    m_results->setColumnWidth(ColStatus, 120);
    m_results->setSorting(ColName);
    /* A single click on a row selects it; only the box itself ticks it. */
    m_results->setSelectionMode(TQListView::Single);

    m_tabs = new KTabWidget(vsplit, "infotabs");
    m_detailsView = new KTextBrowser(m_tabs, "details");
    m_detailsView->setNotifyClick(false);
    m_filesView = new KTextBrowser(m_tabs, "files");
    m_filesView->setTextFormat(TQt::PlainText);
    m_dependsView = new TDEListView(m_tabs, "depends");
    m_requiredView = new TDEListView(m_tabs, "requiredby");
    TDEListView *lists[] = { m_dependsView, m_requiredView };
    for (TDEListView *list : lists) {
        list->addColumn(i18n("Package"));
        list->addColumn(i18n("Version"));
        list->addColumn(i18n("Summary"));
        list->addColumn(i18n("Status"));
        list->setAllColumnsShowFocus(true);
        list->setRootIsDecorated(false);
    }
    m_tabs->addTab(m_detailsView, i18n("Details"));
    m_tabs->addTab(m_filesView, i18n("Files"));
    m_tabs->addTab(m_dependsView, i18n("Depends On"));
    m_tabs->addTab(m_requiredView, i18n("Required By"));

    TQValueList<int> hsizes;
    hsizes << 170 << 650;
    hsplit->setSizes(hsizes);
    hsplit->setResizeMode(m_groups, TQSplitter::KeepSize);
    TQValueList<int> vsizes;
    vsizes << 300 << 180;
    vsplit->setSizes(vsizes);

    m_status = new TQLabel(i18n("Type a search or choose a category to see packages."), this, "browsestatus");
    top->addWidget(m_status);

    m_flushTimer = new TQTimer(this);

    connect(m_search, TQ_SIGNAL(returnPressed()), TQ_SLOT(startSearch()));
    connect(m_find, TQ_SIGNAL(clicked()), TQ_SLOT(startSearch()));
    connect(m_mode, TQ_SIGNAL(activated(int)), TQ_SLOT(reload()));
    connect(m_stateFilter, TQ_SIGNAL(activated(int)), TQ_SLOT(reload()));
    connect(m_guiOnly, TQ_SIGNAL(toggled(bool)), TQ_SLOT(reload()));
    connect(m_newestOnly, TQ_SIGNAL(toggled(bool)), TQ_SLOT(reload()));
    connect(m_groups, TQ_SIGNAL(clicked(TQListViewItem *)), TQ_SLOT(groupActivated(TQListViewItem *)));
    connect(m_groups, TQ_SIGNAL(returnPressed(TQListViewItem *)), TQ_SLOT(groupActivated(TQListViewItem *)));
    connect(m_results, TQ_SIGNAL(selectionChanged()), TQ_SLOT(onSelectionChanged()));
    connect(m_tabs, TQ_SIGNAL(currentChanged(TQWidget *)), TQ_SLOT(onTabChanged()));
    connect(m_flushTimer, TQ_SIGNAL(timeout()), TQ_SLOT(flushPending()));
    connect(m_queue, TQ_SIGNAL(changed()), TQ_SLOT(syncWithQueue()));
}

void BrowsePage::focusSearch()
{
    m_search->setFocus();
    m_search->selectAll();
}

void BrowsePage::populateGroups()
{
    PkDaemon *daemon = PkDaemon::self();
    m_groups->clear();
    /* Filters the backend does not offer are hidden, rather than shown and dead. */
    const bool hasGui = daemon->filters() & Pk::bit(Pk::FilterGui);
    const bool hasNewest = daemon->filters() & Pk::bit(Pk::FilterNewest);
    m_guiOnly->setEnabled(hasGui);
    m_newestOnly->setEnabled(hasNewest);
    if (hasGui) m_guiOnly->show(); else m_guiOnly->hide();
    if (hasNewest) m_newestOnly->show(); else m_newestOnly->hide();

    for (uint g = Pk::GroupAccessibility; g < Pk::GroupLast; ++g) {
        if (!(daemon->groups() & Pk::bit(g))) {
            continue;
        }
        TQListViewItem *item = new TQListViewItem(m_groups, Pk::groupLabel(g));
        TQPixmap icon = TDEGlobal::iconLoader()->loadIcon(Pk::groupIcon(g), TDEIcon::Small, 0,
                                                          TDEIcon::DefaultState, nullptr, true);
        item->setPixmap(0, icon.isNull() ? SmallIcon("folder") : icon);
        item->setText(1, Pk::groupId(g));
    }
    m_groups->setSorting(0);
    m_groups->sort();
}

Pk::Bitfield BrowsePage::currentFilters() const
{
    Pk::Bitfield filters = 0;
    switch (m_stateFilter->currentItem()) {
    case StateInstalled: filters |= Pk::bit(Pk::FilterInstalled); break;
    case StateAvailable: filters |= Pk::bit(Pk::FilterNotInstalled); break;
    default: break;
    }
    if (m_guiOnly->isEnabled() && m_guiOnly->isChecked()) {
        filters |= Pk::bit(Pk::FilterGui);
    }
    if (m_newestOnly->isEnabled() && m_newestOnly->isChecked()) {
        filters |= Pk::bit(Pk::FilterNewest);
    }
    return filters ? filters : Pk::bit(Pk::FilterNone);
}

void BrowsePage::startSearch()
{
    const TQString text = m_search->text().stripWhiteSpace();
    if (text.isEmpty()) {
        return;
    }
    m_lastKind = TextQuery;
    m_lastText = text;
    m_lastMode = m_mode->currentItem();
    m_groups->clearSelection();
    runQuery();
}

void BrowsePage::groupActivated(TQListViewItem *item)
{
    if (!item) {
        return;
    }
    m_lastKind = GroupQuery;
    m_lastGroup = item->text(1);
    m_search->clear();
    runQuery();
}

void BrowsePage::reload()
{
    if (m_lastKind == TextQuery) {
        m_lastMode = m_mode->currentItem();
    }
    if (m_lastKind != NoQuery) {
        runQuery();
    }
}

void BrowsePage::runQuery()
{
    QueryHelper::abandon(m_query, this);
    QueryHelper::abandon(m_info, this);
    m_flushTimer->stop();
    m_pending.clear();
    m_itemsByKey.clear();
    m_results->clear();
    clearInfo();
    m_errorCode = 0;
    m_errorDetails = TQString();
    m_found = 0;
    m_status->setText(i18n("Searching..."));
    m_results->setSorting(-1);

    m_query = new PkTransaction(this);
    connect(m_query, TQ_SIGNAL(package(const PkPackage &)), TQ_SLOT(onPackage(const PkPackage &)));
    connect(m_query, TQ_SIGNAL(errorCode(uint, const TQString &)), TQ_SLOT(onQueryError(uint, const TQString &)));
    connect(m_query, TQ_SIGNAL(finished(uint, uint)), TQ_SLOT(onQueryFinished(uint, uint)));

    const Pk::Bitfield filters = currentFilters();
    if (m_lastKind == GroupQuery) {
        m_query->searchGroups(filters, TQStringList(m_lastGroup));
        return;
    }
    const TQStringList terms = TQStringList::split(' ', m_lastText);
    switch (m_lastMode) {
    case ModeDescription: m_query->searchDetails(filters, terms); break;
    case ModeFile:        m_query->searchFiles(filters, terms); break;
    default:              m_query->searchNames(filters, terms); break;
    }
}

void BrowsePage::onPackage(const PkPackage &package)
{
    m_pending.append(package);
    if (!m_flushTimer->isActive()) {
        m_flushTimer->start(200);
    }
}

void BrowsePage::flushPending()
{
    m_flushTimer->stop();
    m_syncing = true;
    m_results->setUpdatesEnabled(false);
    for (PkPackageList::const_iterator it = m_pending.begin(); it != m_pending.end(); ++it) {
        const TQString key = itemKey(*it);
        TQMap<TQString, PackageItem *>::iterator existing = m_itemsByKey.find(key);
        if (existing != m_itemsByKey.end()) {
            /* The same package as both installed and available: show the installed
               one, since a newer version is offered on the Updates page. */
            if (!Pk::infoIsInstalled((*it).info) && existing.data()->installed()) {
                continue;
            }
            existing.data()->setPackage(*it);
            continue;
        }
        PackageItem *item = new PackageItem(m_results, this, *it);
        m_itemsByKey.insert(key, item);
        ++m_found;
    }
    m_pending.clear();
    m_results->setUpdatesEnabled(true);
    m_syncing = false;
    syncWithQueue();
    m_results->triggerUpdate();
}

void BrowsePage::onQueryError(uint code, const TQString &details)
{
    m_errorCode = code;
    m_errorDetails = details;
}

void BrowsePage::onQueryFinished(uint exit, uint)
{
    QueryHelper::release(m_query);
    flushPending();
    m_results->setSorting(ColName);
    m_results->sort();
    if (m_errorCode && exit != Pk::ExitSuccess) {
        m_status->setText(Pk::errorLabel(m_errorCode));
        m_status->setTextFormat(TQt::PlainText);
        return;
    }
    if (m_found == 0) {
        m_status->setText(i18n("No packages found."));
    } else {
        m_status->setText(i18n("1 package found.", "%n packages found.", m_found));
    }
}

void BrowsePage::itemToggled(PackageItem *item, bool on)
{
    if (m_syncing) {
        return;
    }
    const PkPackage &p = item->package();
    ChangeQueue::Action action = ChangeQueue::None;
    if (item->installed() && !on) {
        action = ChangeQueue::Remove;
    } else if (!item->installed() && on) {
        action = ChangeQueue::Install;
    }
    m_queue->set(p.id, action);
    m_results->setSelected(item, true);
}

void BrowsePage::syncWithQueue()
{
    m_syncing = true;
    for (TQListViewItemIterator it(m_results); it.current(); ++it) {
        PackageItem *item = static_cast<PackageItem *>(it.current());
        const ChangeQueue::Action action = m_queue->action(item->package().id);
        const bool installed = item->installed();
        item->setOn(action == ChangeQueue::Install || (installed && action != ChangeQueue::Remove));
        TQString status;
        switch (action) {
        case ChangeQueue::Install: status = i18n("Will be installed"); break;
        case ChangeQueue::Remove:  status = i18n("Will be removed"); break;
        case ChangeQueue::Update:  status = i18n("Will be updated"); break;
        default: status = installed ? Pk::infoLabel(Pk::InfoInstalled) : Pk::infoLabel(Pk::InfoAvailable);
        }
        item->setText(ColStatus, status);
    }
    m_syncing = false;
}

void BrowsePage::clearInfo()
{
    m_detailsView->clear();
    m_filesView->clear();
    m_dependsView->clear();
    m_requiredView->clear();
    m_infoPage = -1;
}

void BrowsePage::onSelectionChanged()
{
    clearInfo();
    requestInfo();
}

void BrowsePage::onTabChanged()
{
    requestInfo();
}

void BrowsePage::requestInfo()
{
    QueryHelper::abandon(m_info, this);
    PackageItem *item = static_cast<PackageItem *>(m_results->selectedItem());
    if (!item) {
        return;
    }
    const int page = m_tabs->currentPageIndex();
    const TQStringList ids(item->package().id);
    const Pk::Bitfield noFilter = Pk::bit(Pk::FilterNone);

    m_info = new PkTransaction(this);
    connect(m_info, TQ_SIGNAL(finished(uint, uint)), TQ_SLOT(onInfoFinished(uint, uint)));
    m_infoPage = page;
    switch (page) {
    case 0:
        connect(m_info, TQ_SIGNAL(details(const PkDetails &)), TQ_SLOT(onDetails(const PkDetails &)));
        m_info->getDetails(ids);
        break;
    case 1:
        m_filesView->setText(i18n("Getting file list..."));
        connect(m_info, TQ_SIGNAL(files(const TQString &, const TQStringList &)),
                TQ_SLOT(onFiles(const TQString &, const TQStringList &)));
        m_info->getFiles(ids);
        break;
    case 2:
        m_infoList = m_dependsView;
        m_dependsView->clear();
        connect(m_info, TQ_SIGNAL(package(const PkPackage &)), TQ_SLOT(onRelatedPackage(const PkPackage &)));
        m_info->dependsOn(noFilter, ids, false);
        break;
    default:
        m_infoList = m_requiredView;
        m_requiredView->clear();
        connect(m_info, TQ_SIGNAL(package(const PkPackage &)), TQ_SLOT(onRelatedPackage(const PkPackage &)));
        m_info->requiredBy(noFilter, ids, false);
        break;
    }
}

void BrowsePage::onDetails(const PkDetails &d)
{
    using QueryHelper::escapeHtml;
    const PackageId id(d.id);
    PackageItem *item = static_cast<PackageItem *>(m_results->selectedItem());
    TQString html = "<h3>" + escapeHtml(id.name()) + "</h3>";
    const TQString summary = !d.summary.isEmpty() ? d.summary : (item ? item->package().summary : TQString());
    if (!summary.isEmpty()) {
        html += "<p><b>" + escapeHtml(summary) + "</b></p>";
    }
    if (!d.description.isEmpty()) {
        html += "<p>" + escapeHtml(d.description).replace('\n', "<br>") + "</p>";
    }
    html += "<table cellspacing=\"2\">";
    const auto row = [](const TQString &label, const TQString &value) {
        return value.isEmpty() ? TQString()
                               : "<tr><td><b>" + escapeHtml(label) + "</b>&nbsp;</td><td>" + value + "</td></tr>";
    };
    html += row(i18n("Version:"), escapeHtml(id.version()));
    html += row(i18n("Architecture:"), escapeHtml(id.arch()));
    html += row(i18n("Repository:"), escapeHtml(id.origin()));
    html += row(i18n("Category:"), d.group ? escapeHtml(Pk::groupLabel(d.group)) : TQString());
    html += row(i18n("License:"), escapeHtml(d.license));
    html += row(i18n("Size:"), d.size ? escapeHtml(TDEIO::convertSize(d.size)) : TQString());
    if (d.url.startsWith("http://") || d.url.startsWith("https://")) {
        const TQString url = escapeHtml(d.url);
        html += row(i18n("Website:"), "<a href=\"" + TQString(url).replace('"', "&quot;") + "\">" + url + "</a>");
    }
    html += "</table>";
    m_detailsView->setText(html);
}

void BrowsePage::onFiles(const TQString &, const TQStringList &files)
{
    m_filesView->setText(files.isEmpty() ? i18n("This package has no file list.") : files.join("\n"));
}

void BrowsePage::onRelatedPackage(const PkPackage &package)
{
    if (!m_infoList) {
        return;
    }
    const PackageId id(package.id);
    TQListViewItem *item = new TQListViewItem(m_infoList, id.name(), id.version(), package.summary,
                                              Pk::infoLabel(package.info));
    item->setPixmap(0, PackageIcon::small(id.name()));
}

void BrowsePage::onInfoFinished(uint exit, uint)
{
    const int page = m_infoPage;
    if (exit != Pk::ExitSuccess && page == 1) {
        m_filesView->setText(i18n("The file list is not available for this package."));
    }
    if (page == 1 && exit == Pk::ExitSuccess && m_filesView->text() == i18n("Getting file list...")) {
        m_filesView->setText(i18n("This package has no file list."));
    }
    QueryHelper::release(m_info);
}

#include "browsepage.moc"
