/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TDEPK_MAINWINDOW_H
#define TDEPK_MAINWINDOW_H

#include <tqstringlist.h>

#include <tdemainwindow.h>

class ApplyFlow;
class BrowsePage;
class ChangeQueue;
class HistoryPage;
class KPushButton;
class ReposPage;
class TDEListBox;
class TQLabel;
class TQWidgetStack;
class UpdatesPage;

class MainWindow : public TDEMainWindow
{
    TQ_OBJECT
public:
    enum Page { BrowsePageIndex, UpdatesPageIndex, HistoryPageIndex, ReposPageIndex };

    MainWindow();

    void showPage(int page);
    /* Both return at once, because they are called while another process waits
       for this one over DCOP. The work runs from the event loop. */
    void installFiles(const TQStringList &paths);
    void showSettings();

public slots:
    void showBrowse() { showPage(BrowsePageIndex); }
    void showUpdates() { showPage(UpdatesPageIndex); }
    void showHistory() { showPage(HistoryPageIndex); }
    void showRepos() { showPage(ReposPageIndex); }

protected:
    bool queryClose() override;

private slots:
    void onSidebarChanged(int index);
    void onQueueChanged();
    void onUpdatesCount(uint count);
    void apply();
    void clearQueue();
    void onFlowDone(bool changed);
    void onDaemonProperties();
    void onDaemonUnavailable(const TQString &reason);
    void reportUnavailable();
    void onDaemonChanged();
    void refreshAll();
    void configure();
    void startPendingFiles();

private:
    void buildMenus();
    void setSidebarText(int index, const TQString &text, const TQString &icon);

    ChangeQueue *m_queue;
    TDEListBox *m_sidebar;
    TQWidgetStack *m_stack;
    BrowsePage *m_browse;
    UpdatesPage *m_updates;
    HistoryPage *m_history;
    ReposPage *m_repos;
    TQLabel *m_pending;
    KPushButton *m_apply;
    KPushButton *m_clear;
    ApplyFlow *m_flow;
    TQStringList m_pendingFiles;
    TQString m_unavailableReason;
    bool m_loaded;
};

#endif
