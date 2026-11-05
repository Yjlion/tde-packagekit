/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TDEPK_HISTORYPAGE_H
#define TDEPK_HISTORYPAGE_H

#include <tqwidget.h>

#include "pktypes.h"

class KTextBrowser;
class PkTransaction;
class TDEListView;
class TQLabel;

class HistoryPage : public TQWidget
{
    TQ_OBJECT
public:
    explicit HistoryPage(TQWidget *parent = nullptr);

public slots:
    void load();

private slots:
    void onEntry(const PkHistoryEntry &entry);
    void onError(uint code, const TQString &details);
    void onFinished(uint exit, uint runtime);
    void onSelectionChanged();

private:
    TQLabel *m_status;
    TDEListView *m_list;
    KTextBrowser *m_detail;
    PkTransaction *m_query;
    uint m_errorCode;
    uint m_shown;
};

#endif
