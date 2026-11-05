/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TDEPK_EULADIALOG_H
#define TDEPK_EULADIALOG_H

#include <kdialogbase.h>

#include "pktypes.h"

class EulaDialog : public KDialogBase
{
    TQ_OBJECT
public:
    EulaDialog(const PkEula &eula, TQWidget *parent = nullptr);

private slots:
    void acceptToggled(bool on);
};

#endif
