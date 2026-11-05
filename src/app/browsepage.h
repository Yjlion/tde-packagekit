/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TDEPK_BROWSEPAGE_H
#define TDEPK_BROWSEPAGE_H

#include <tqmap.h>
#include <tqstringlist.h>
#include <tqwidget.h>

#include "pkenums.h"
#include "pktypes.h"

class ChangeQueue;
class KComboBox;
class KLineEdit;
class KPushButton;
class KTabWidget;
class KTextBrowser;
class PackageItem;
class PkTransaction;
class TDEListView;
class TQCheckBox;
class TQLabel;
class TQListViewItem;
class TQTimer;

class BrowsePage : public TQWidget
{
    TQ_OBJECT
public:
    BrowsePage(ChangeQueue *queue, TQWidget *parent = nullptr);

    void itemToggled(PackageItem *item, bool on);
    void focusSearch();

public slots:
    void populateGroups();
    /* Runs the last search again, for example after changes were applied. */
    void reload();

private slots:
    void startSearch();
    void groupActivated(TQListViewItem *item);
    void onPackage(const PkPackage &package);
    void onQueryError(uint code, const TQString &details);
    void onQueryFinished(uint exit, uint runtime);
    void flushPending();
    void onSelectionChanged();
    void onTabChanged();
    void onDetails(const PkDetails &details);
    void onFiles(const TQString &id, const TQStringList &files);
    void onRelatedPackage(const PkPackage &package);
    void onInfoFinished(uint exit, uint runtime);
    void syncWithQueue();

private:
    enum QueryKind { NoQuery, TextQuery, GroupQuery };

    Pk::Bitfield currentFilters() const;
    void runQuery();
    void requestInfo();
    void clearInfo();

    ChangeQueue *m_queue;
    KLineEdit *m_search;
    KComboBox *m_mode;
    KPushButton *m_find;
    KComboBox *m_stateFilter;
    TQCheckBox *m_guiOnly;
    TQCheckBox *m_newestOnly;
    TDEListView *m_groups;
    TDEListView *m_results;
    TQLabel *m_status;
    KTabWidget *m_tabs;
    KTextBrowser *m_detailsView;
    KTextBrowser *m_filesView;
    TDEListView *m_dependsView;
    TDEListView *m_requiredView;

    PkTransaction *m_query;
    PkTransaction *m_info;
    TQTimer *m_flushTimer;
    PkPackageList m_pending;
    TQMap<TQString, PackageItem *> m_itemsByKey;
    uint m_errorCode;
    TQString m_errorDetails;
    uint m_found;
    bool m_syncing;

    QueryKind m_lastKind;
    TQString m_lastText;
    int m_lastMode;
    TQString m_lastGroup;
    int m_infoPage;
    TDEListView *m_infoList;
};

#endif
