/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "mainwindow.h"

#include <tqframe.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlistbox.h>
#include <tqtimer.h>
#include <tqwidgetstack.h>

#include <khelpmenu.h>
#include <kiconloader.h>
#include <kpushbutton.h>
#include <kstatusbar.h>
#include <tdestdaccel.h>
#include <kstdaction.h>
#include <kstdguiitem.h>
#include <tdeaction.h>
#include <tdeapplication.h>
#include <tdelistbox.h>
#include <tdelocale.h>
#include <tdemenubar.h>
#include <tdemessagebox.h>
#include <tdepopupmenu.h>

#include "applyflow.h"
#include "browsepage.h"
#include "errordialog.h"
#include "changequeue.h"
#include "historypage.h"
#include "pkdaemon.h"
#include "repospage.h"
#include "settingsdialog.h"
#include "updatespage.h"

MainWindow::MainWindow()
    : TDEMainWindow(nullptr, "mainwindow"), m_flow(nullptr), m_loaded(false)
{
    setCaption(i18n("Software Management"), false);
    m_queue = new ChangeQueue(this);

    TQWidget *central = new TQWidget(this);
    setCentralWidget(central);
    TQVBoxLayout *outer = new TQVBoxLayout(central, 8, 8);

    TQHBoxLayout *body = new TQHBoxLayout(outer, 8);

    m_sidebar = new TDEListBox(central, "sidebar");
    m_sidebar->setFixedWidth(170);
    body->addWidget(m_sidebar);

    m_stack = new TQWidgetStack(central, "pages");
    body->addWidget(m_stack, 1);

    m_browse = new BrowsePage(m_queue, m_stack);
    m_updates = new UpdatesPage(m_queue, m_stack);
    m_history = new HistoryPage(m_stack);
    m_repos = new ReposPage(m_stack);
    m_stack->addWidget(m_browse, BrowsePageIndex);
    m_stack->addWidget(m_updates, UpdatesPageIndex);
    m_stack->addWidget(m_history, HistoryPageIndex);
    m_stack->addWidget(m_repos, ReposPageIndex);

    m_sidebar->insertItem(new TQListBoxPixmap(DesktopIcon("system-software-install", 32), i18n("Browse")));
    m_sidebar->insertItem(new TQListBoxPixmap(DesktopIcon("system-software-update", 32), i18n("Updates")));
    m_sidebar->insertItem(new TQListBoxPixmap(DesktopIcon("history", 32), i18n("History")));
    m_sidebar->insertItem(new TQListBoxPixmap(DesktopIcon("folder_green", 32), i18n("Repositories")));

    TQFrame *line = new TQFrame(central);
    line->setFrameShape(TQFrame::HLine);
    line->setFrameShadow(TQFrame::Sunken);
    outer->addWidget(line);

    TQHBoxLayout *bottom = new TQHBoxLayout(outer, 8);
    m_pending = new TQLabel(central, "pending");
    m_clear = new KPushButton(KGuiItem(i18n("Clea&r"), "edit-clear"), central, "clear");
    m_apply = new KPushButton(KGuiItem(i18n("&Apply"), "ok"), central, "apply");
    m_apply->setDefault(false);
    m_apply->setAutoDefault(false);
    bottom->addWidget(m_pending, 1);
    bottom->addWidget(m_clear);
    bottom->addWidget(m_apply);

    buildMenus();

    statusBar()->insertItem(TQString(), 1, 1);

    connect(m_sidebar, TQ_SIGNAL(highlighted(int)), TQ_SLOT(onSidebarChanged(int)));
    connect(m_queue, TQ_SIGNAL(changed()), TQ_SLOT(onQueueChanged()));
    connect(m_apply, TQ_SIGNAL(clicked()), TQ_SLOT(apply()));
    connect(m_clear, TQ_SIGNAL(clicked()), TQ_SLOT(clearQueue()));
    connect(m_updates, TQ_SIGNAL(countChanged(uint)), TQ_SLOT(onUpdatesCount(uint)));
    connect(m_repos, TQ_SIGNAL(reposChanged()), m_browse, TQ_SLOT(reload()));

    PkDaemon *daemon = PkDaemon::self();
    connect(daemon, TQ_SIGNAL(propertiesChanged()), TQ_SLOT(onDaemonProperties()));
    connect(daemon, TQ_SIGNAL(unavailable(const TQString &)), TQ_SLOT(onDaemonUnavailable(const TQString &)));
    connect(daemon, TQ_SIGNAL(updatesChanged()), TQ_SLOT(onDaemonChanged()));
    connect(daemon, TQ_SIGNAL(repoListChanged()), m_repos, TQ_SLOT(load()));

    onQueueChanged();
    resize(1000, 680);
    m_sidebar->setCurrentItem(BrowsePageIndex);
    m_browse->focusSearch();
    daemon->refreshProperties();
}

void MainWindow::buildMenus()
{
    TDEActionCollection *ac = actionCollection();
    TDEPopupMenu *file = new TDEPopupMenu(this);
    (new TDEAction(i18n("&Check for Updates"), "reload", TDEStdAccel::shortcut(TDEStdAccel::Reload),
                   this, TQ_SLOT(refreshAll()), ac, "refresh"))->plug(file);
    file->insertSeparator();
    KStdAction::quit(this, TQ_SLOT(close()), ac)->plug(file);
    menuBar()->insertItem(i18n("&File"), file);

    TDEPopupMenu *view = new TDEPopupMenu(this);
    (new TDEAction(i18n("&Browse"), "system-software-install", TDEShortcut("Ctrl+1"), this, TQ_SLOT(showBrowse()), ac, "page_browse"))->plug(view);
    (new TDEAction(i18n("&Updates"), "system-software-update", TDEShortcut("Ctrl+2"), this, TQ_SLOT(showUpdates()), ac, "page_updates"))->plug(view);
    (new TDEAction(i18n("&History"), "history", TDEShortcut("Ctrl+3"), this, TQ_SLOT(showHistory()), ac, "page_history"))->plug(view);
    (new TDEAction(i18n("&Repositories"), "folder_green", TDEShortcut("Ctrl+4"), this, TQ_SLOT(showRepos()), ac, "page_repos"))->plug(view);
    menuBar()->insertItem(i18n("&View"), view);

    TDEPopupMenu *settings = new TDEPopupMenu(this);
    KStdAction::preferences(this, TQ_SLOT(configure()), ac)->plug(settings);
    menuBar()->insertItem(i18n("&Settings"), settings);

    TQPopupMenu *help = helpMenu();
    /* Trinity's standard help menu adds "Report Bug...", which files reports in
       Trinity's own bug tracker. This isn't a Trinity project, so the item is
       taken out. Delete the next line to bring it back. */
    help->removeItem(KHelpMenu::menuReportBug);
    menuBar()->insertItem(i18n("&Help"), help);
}

void MainWindow::showPage(int page)
{
    m_sidebar->setCurrentItem(page);
    onSidebarChanged(page);
}

void MainWindow::onSidebarChanged(int index)
{
    m_stack->raiseWidget(index);
    switch (index) {
    case BrowsePageIndex:
        m_browse->focusSearch();
        break;
    case UpdatesPageIndex:
        m_updates->activate();
        break;
    case HistoryPageIndex:
        m_history->load();
        break;
    case ReposPageIndex:
        m_repos->load();
        break;
    }
}

void MainWindow::onQueueChanged()
{
    const uint install = m_queue->count(ChangeQueue::Install);
    const uint remove = m_queue->count(ChangeQueue::Remove);
    const uint update = m_queue->count(ChangeQueue::Update);
    TQStringList parts;
    if (install) parts << i18n("1 to install", "%n to install", install);
    if (remove)  parts << i18n("1 to remove", "%n to remove", remove);
    if (update)  parts << i18n("1 to update", "%n to update", update);
    m_pending->setText(parts.isEmpty() ? i18n("No changes pending.")
                                       : i18n("Changes pending: %1").arg(parts.join(", ")));
    const bool any = !m_queue->isEmpty();
    m_apply->setEnabled(any && !m_flow);
    m_clear->setEnabled(any && !m_flow);
}

void MainWindow::onUpdatesCount(uint count)
{
    TQListBoxItem *old = m_sidebar->item(UpdatesPageIndex);
    const int current = m_sidebar->currentItem();
    const TQString text = count ? i18n("Updates (%1)").arg(count) : i18n("Updates");
    m_sidebar->blockSignals(true);
    m_sidebar->changeItem(DesktopIcon("system-software-update", 32), text, UpdatesPageIndex);
    m_sidebar->setCurrentItem(current);
    m_sidebar->blockSignals(false);
    Q_UNUSED(old);
}

void MainWindow::apply()
{
    if (m_flow || m_queue->isEmpty()) {
        return;
    }
    m_flow = new ApplyFlow(this);
    connect(m_flow, TQ_SIGNAL(done(bool)), TQ_SLOT(onFlowDone(bool)));
    onQueueChanged();
    m_flow->applyQueue(m_queue);
}

void MainWindow::installFiles(const TQStringList &paths)
{
    if (paths.isEmpty()) {
        return;
    }
    m_pendingFiles = paths;
    TQTimer::singleShot(0, this, TQ_SLOT(startPendingFiles()));
}

void MainWindow::startPendingFiles()
{
    const TQStringList paths = m_pendingFiles;
    m_pendingFiles.clear();
    if (paths.isEmpty()) {
        return;
    }
    show();
    raise();
    if (m_flow) {
        KMessageBox::sorry(this, i18n("Wait for the current changes to finish, then open the file again."));
        return;
    }
    m_flow = new ApplyFlow(this);
    connect(m_flow, TQ_SIGNAL(done(bool)), TQ_SLOT(onFlowDone(bool)));
    onQueueChanged();
    m_flow->installFiles(paths);
}

void MainWindow::showSettings()
{
    TQTimer::singleShot(0, this, TQ_SLOT(configure()));
}

void MainWindow::clearQueue()
{
    /* Otherwise the next update check would tick the updates again. Updates the
       user has not been offered yet are left alone. */
    if (m_queue->count(ChangeQueue::Update) > 0) {
        m_updates->selectNone();
    }
    m_queue->clear();
}

void MainWindow::onFlowDone(bool changed)
{
    m_flow->deleteLater();
    m_flow = nullptr;
    if (changed) {
        m_browse->reload();
        m_updates->load();
        if (m_stack->visibleWidget() == m_history) {
            m_history->load();
        }
    }
    onQueueChanged();
}

void MainWindow::onDaemonProperties()
{
    PkDaemon *daemon = PkDaemon::self();
    statusBar()->changeItem(i18n("PackageKit %1, %2 backend").arg(daemon->version()).arg(daemon->backendName()), 1);
    if (!m_loaded) {
        m_loaded = true;
        m_browse->populateGroups();
        m_updates->load();
    }
}

void MainWindow::onDaemonUnavailable(const TQString &reason)
{
    m_loaded = false;
    statusBar()->changeItem(i18n("PackageKit is not available"), 1);
    m_unavailableReason = reason;
    /* The message box can't open inside the D-Bus callback. See pktransaction.h. */
    TQTimer::singleShot(0, this, TQ_SLOT(reportUnavailable()));
}

void MainWindow::reportUnavailable()
{
    ErrorDialog::report(this, i18n("PackageKit Not Available"),
                        i18n("The PackageKit service could not be reached, so software "
                             "cannot be managed right now. Check that the packagekit "
                             "package is installed."),
                        m_unavailableReason, ErrorDialog::Sorry);
}

void MainWindow::onDaemonChanged()
{
    if (!m_flow) {
        m_updates->load();
    }
}

void MainWindow::refreshAll()
{
    showPage(UpdatesPageIndex);
    m_updates->refreshCache();
}

void MainWindow::configure()
{
    SettingsDialog dialog(this);
    dialog.exec();
}

bool MainWindow::queryClose()
{
    if (m_flow) {
        KMessageBox::sorry(this, i18n("Software changes are being applied. Wait for them to finish before closing."));
        return false;
    }
    return true;
}

#include "mainwindow.moc"
