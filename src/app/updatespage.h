/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TDEPK_UPDATESPAGE_H
#define TDEPK_UPDATESPAGE_H

#include <tqmap.h>
#include <tqstringlist.h>
#include <tqwidget.h>

#include "pktypes.h"

class ChangeQueue;
class KPushButton;
class KTextBrowser;
class PkTransaction;
class TDEListView;
class TQLabel;
class TQListViewItem;
class UpdateItem;

class UpdatesPage : public TQWidget
{
    TQ_OBJECT
public:
    UpdatesPage(ChangeQueue *queue, TQWidget *parent = nullptr);

    void itemToggled(UpdateItem *item, bool on);
    /* The page became visible: ticks the updates the first time the user sees them. */
    void activate();
    uint updateCount() const { return m_count; }

public slots:
    void load();
    void refreshCache();
    void selectAll();
    void selectNone();

signals:
    void countChanged(uint count);

private slots:
    void onPackage(const PkPackage &package);
    void onLoadError(uint code, const TQString &details);
    void onLoadFinished(uint exit, uint runtime);
    void onRefreshError(uint code, const TQString &details);
    void onRefreshProgress(uint percentage);
    void onRefreshStatus(uint status);
    void onRefreshFinished(uint exit, uint runtime);
    void afterRefresh();
    void onSelectionChanged();
    void onUpdateDetail(const PkUpdateDetail &detail);
    void onDetailFinished(uint exit, uint runtime);
    void onTimeSince(uint role, uint seconds);
    void syncWithQueue();

private:
    TQListViewItem *headerFor(uint rank);
    void populate();
    void updateSummary();
    TQStringList allSelectableIds() const;

    ChangeQueue *m_queue;
    TQLabel *m_summary;
    TQLabel *m_lastChecked;
    KPushButton *m_refresh;
    KPushButton *m_all;
    KPushButton *m_none;
    TDEListView *m_list;
    KTextBrowser *m_detail;

    PkTransaction *m_load;
    PkTransaction *m_refreshTx;
    PkTransaction *m_detailTx;
    PkPackageList m_packages;
    TQMap<uint, TQListViewItem *> m_headers;
    TQStringList m_userUnticked;
    uint m_count;
    uint m_errorCode;
    TQString m_errorDetails;
    bool m_syncing;
    bool m_activated;
    bool m_gotDetail;
    bool m_refreshFailed;
};

#endif
