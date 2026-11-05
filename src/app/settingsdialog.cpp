/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "settingsdialog.h"

#include <tqbuttongroup.h>
#include <tqcheckbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqradiobutton.h>
#include <tqvbox.h>

#include <dcopclient.h>
#include <kcombobox.h>
#include <krun.h>
#include <tdeapplication.h>
#include <tdelocale.h>

#include "tdepksettings.h"

static const int Intervals[] = { 6, 24, 168, 720, 0 };
static const int IntervalCount = sizeof(Intervals) / sizeof(Intervals[0]);

SettingsDialog::SettingsDialog(TQWidget *parent)
    : KDialogBase(parent, "settingsdialog", true, i18n("Configure Software Updates"), Ok | Cancel, Ok, true)
{
    TQVBox *page = makeVBoxMainWidget();

    m_enabled = new TQCheckBox(i18n("&Run the update notifier in the system tray"), page, "notifierenabled");
    m_hideIdle = new TQCheckBox(i18n("&Hide the tray icon when there are no updates"), page, "hidewhenidle");

    TQWidget *row = new TQWidget(page);
    TQHBoxLayout *rowLayout = new TQHBoxLayout(row, 0, 6);
    TQLabel *label = new TQLabel(i18n("Check for &updates:"), row);
    m_interval = new KComboBox(row, "checkinterval");
    label->setBuddy(m_interval);
    m_interval->insertItem(i18n("Every 6 hours"));
    m_interval->insertItem(i18n("Daily"));
    m_interval->insertItem(i18n("Weekly"));
    m_interval->insertItem(i18n("Monthly"));
    m_interval->insertItem(i18n("Never"));
    rowLayout->addWidget(label);
    rowLayout->addWidget(m_interval);
    rowLayout->addStretch(1);

    TQButtonGroup *group = new TQButtonGroup(1, TQt::Horizontal, i18n("Notify Me About"), page);
    m_notifyAll = new TQRadioButton(i18n("&All updates"), group, "notifyall");
    m_notifySecurity = new TQRadioButton(i18n("&Security updates only"), group, "notifysecurity");

    TQLabel *note = new TQLabel(i18n("Updates are never installed automatically. "
                                     "You choose when to install them."), page);
    note->setAlignment(TQt::WordBreak);

    m_enabled->setChecked(TdePkSettings::notifierEnabled());
    m_hideIdle->setChecked(TdePkSettings::hideWhenIdle());
    const int hours = TdePkSettings::checkIntervalHours();
    int index = 1;
    for (int i = 0; i < IntervalCount; ++i) {
        if (Intervals[i] == hours) {
            index = i;
        }
    }
    m_interval->setCurrentItem(index);
    (TdePkSettings::notifySecurityOnly() ? m_notifySecurity : m_notifyAll)->setChecked(true);

    connect(m_enabled, TQ_SIGNAL(toggled(bool)), TQ_SLOT(enabledToggled(bool)));
    enabledToggled(m_enabled->isChecked());
    resize(430, 260);
}

void SettingsDialog::enabledToggled(bool on)
{
    m_hideIdle->setEnabled(on);
    m_interval->setEnabled(on);
    m_notifyAll->setEnabled(on);
    m_notifySecurity->setEnabled(on);
}

void SettingsDialog::slotOk()
{
    TdePkSettings::setNotifierEnabled(m_enabled->isChecked());
    TdePkSettings::setCheckIntervalHours(Intervals[m_interval->currentItem()]);
    TdePkSettings::setNotifySecurityOnly(m_notifySecurity->isChecked());
    TdePkSettings::setHideWhenIdle(m_hideIdle->isChecked());
    DCOPClient *dcop = kapp->dcopClient();
    if (dcop->isApplicationRegistered("tdepackagekit-notifier")) {
        TQByteArray none;
        dcop->send("tdepackagekit-notifier", "notifier", "reloadSettings()", none);
    } else if (m_enabled->isChecked()) {
        /* Turned on now: start it instead of waiting for the next login. */
        KRun::runCommand("tdepackagekit-notifier");
    }
    KDialogBase::slotOk();
}

#include "settingsdialog.moc"
