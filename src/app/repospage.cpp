/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "repospage.h"

#include <tqcheckbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlistview.h>
#include <tqtimer.h>

#include <tdelistview.h>
#include <tdelocale.h>

#include "errordialog.h"
#include "pkenums.h"
#include "pktransaction.h"
#include "queryhelper.h"

class RepoItem : public TQCheckListItem
{
public:
    RepoItem(TQListView *list, ReposPage *page, const PkRepo &repo)
        : TQCheckListItem(list, repo.description.isEmpty() ? repo.id : repo.description, TQCheckListItem::CheckBox),
          m_page(page), m_repoId(repo.id)
    {
        setText(1, repo.id);
    }
    const TQString &repoId() const { return m_repoId; }

protected:
    void stateChange(bool on) override { m_page->itemToggled(this, on); }

private:
    ReposPage *m_page;
    TQString m_repoId;
};

ReposPage::ReposPage(TQWidget *parent)
    : TQWidget(parent, "repospage"),
      m_load(nullptr), m_enable(nullptr), m_errorCode(0), m_populating(false), m_enableSucceeded(false)
{
    TQVBoxLayout *top = new TQVBoxLayout(this, 0, 6);

    TQLabel *intro = new TQLabel(i18n("Software comes from these repositories. Untick one to stop "
                                      "using it. Changing a repository requires administrator rights."),
                                 this);
    intro->setAlignment(TQt::WordBreak);
    top->addWidget(intro);

    m_list = new TDEListView(this, "repos");
    m_list->addColumn(i18n("Repository"));
    m_list->addColumn(i18n("Identifier"));
    m_list->setAllColumnsShowFocus(true);
    m_list->setRootIsDecorated(false);
    m_list->setColumnWidth(0, 420);
    m_list->setColumnWidth(1, 320);
    top->addWidget(m_list, 1);

    TQHBoxLayout *row = new TQHBoxLayout(top, 6);
    m_showDevelopment = new TQCheckBox(i18n("Show &development and source repositories"), this, "showdev");
    row->addWidget(m_showDevelopment);
    row->addStretch(1);
    m_status = new TQLabel(this, "reposstatus");
    row->addWidget(m_status);

    connect(m_showDevelopment, TQ_SIGNAL(toggled(bool)), TQ_SLOT(load()));
}

void ReposPage::load()
{
    QueryHelper::abandon(m_load, this);
    m_populating = true;
    m_list->clear();
    m_populating = false;
    m_errorCode = 0;
    m_status->setText(i18n("Loading repositories..."));
    m_load = new PkTransaction(this);
    connect(m_load, TQ_SIGNAL(repoDetail(const PkRepo &)), TQ_SLOT(onRepo(const PkRepo &)));
    connect(m_load, TQ_SIGNAL(errorCode(uint, const TQString &)), TQ_SLOT(onLoadError(uint, const TQString &)));
    connect(m_load, TQ_SIGNAL(finished(uint, uint)), TQ_SLOT(onLoadFinished(uint, uint)));
    m_load->getRepoList(m_showDevelopment->isChecked() ? Pk::bit(Pk::FilterNone)
                                                       : Pk::bit(Pk::FilterNotDevelopment));
}

void ReposPage::onRepo(const PkRepo &repo)
{
    m_populating = true;
    RepoItem *item = new RepoItem(m_list, this, repo);
    item->setOn(repo.enabled);
    m_populating = false;
}

void ReposPage::onLoadError(uint code, const TQString &details)
{
    m_errorCode = code;
    m_errorDetails = details;
}

void ReposPage::onLoadFinished(uint exit, uint)
{
    QueryHelper::release(m_load);
    if (exit != Pk::ExitSuccess && m_errorCode) {
        m_status->setText(Pk::errorLabel(m_errorCode));
    } else {
        m_status->setText(i18n("1 repository", "%n repositories", m_list->childCount()));
    }
}

void ReposPage::itemToggled(RepoItem *item, bool on)
{
    if (m_populating) {
        return;
    }
    if (m_enable) {
        /* One change at a time: put the tick back. */
        m_populating = true;
        item->setOn(!on);
        m_populating = false;
        return;
    }
    m_errorCode = 0;
    m_errorDetails = TQString();
    m_list->setEnabled(false);
    m_status->setText(on ? i18n("Enabling repository...") : i18n("Disabling repository..."));
    m_enable = new PkTransaction(this);
    connect(m_enable, TQ_SIGNAL(errorCode(uint, const TQString &)), TQ_SLOT(onEnableError(uint, const TQString &)));
    connect(m_enable, TQ_SIGNAL(finished(uint, uint)), TQ_SLOT(onEnableFinished(uint, uint)));
    m_enable->repoEnable(item->repoId(), on);
}

void ReposPage::onEnableError(uint code, const TQString &details)
{
    m_errorCode = code;
    m_errorDetails = details;
}

void ReposPage::onEnableFinished(uint exit, uint)
{
    QueryHelper::release(m_enable);
    m_list->setEnabled(true);
    m_enableSucceeded = exit == Pk::ExitSuccess;
    /* The error box can't open inside the D-Bus callback. See pktransaction.h. */
    TQTimer::singleShot(0, this, TQ_SLOT(afterEnable()));
}

void ReposPage::afterEnable()
{
    if (!m_enableSucceeded) {
        if (m_errorCode) {
            ErrorDialog::report(this, i18n("Could Not Change Repository"), Pk::errorLabel(m_errorCode),
                                m_errorDetails);
        }
    } else {
        emit reposChanged();
    }
    /* Reloading shows the real state, whichever way the change went. */
    load();
}

#include "repospage.moc"
