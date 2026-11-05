/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TDEPK_SETTINGSDIALOG_H
#define TDEPK_SETTINGSDIALOG_H

#include <kdialogbase.h>

class KComboBox;
class TQCheckBox;
class TQRadioButton;

class SettingsDialog : public KDialogBase
{
    TQ_OBJECT
public:
    explicit SettingsDialog(TQWidget *parent = nullptr);

protected:
    void slotOk() override;

private slots:
    void enabledToggled(bool on);

private:
    TQCheckBox *m_enabled;
    KComboBox *m_interval;
    TQRadioButton *m_notifyAll;
    TQRadioButton *m_notifySecurity;
};

#endif
