/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TDEPK_REPOSPAGE_H
#define TDEPK_REPOSPAGE_H

#include <tqwidget.h>

#include "pktypes.h"

class PkTransaction;
class RepoItem;
class TDEListView;
class TQCheckBox;
class TQLabel;

class ReposPage : public TQWidget
{
    TQ_OBJECT
public:
    explicit ReposPage(TQWidget *parent = nullptr);

    void itemToggled(RepoItem *item, bool on);

public slots:
    void load();

signals:
    /* A repository was enabled or disabled, so the available packages changed. */
    void reposChanged();

private slots:
    void onRepo(const PkRepo &repo);
    void onLoadError(uint code, const TQString &details);
    void onLoadFinished(uint exit, uint runtime);
    void onEnableError(uint code, const TQString &details);
    void onEnableFinished(uint exit, uint runtime);
    void afterEnable();

private:
    TQLabel *m_status;
    TQCheckBox *m_showDevelopment;
    TDEListView *m_list;
    PkTransaction *m_load;
    PkTransaction *m_enable;
    uint m_errorCode;
    TQString m_errorDetails;
    bool m_populating;
    bool m_enableSucceeded;
};

#endif
