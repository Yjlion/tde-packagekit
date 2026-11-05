/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TDEPK_CONFIRMCHANGESDIALOG_H
#define TDEPK_CONFIRMCHANGESDIALOG_H

#include <kdialogbase.h>

#include "pktypes.h"

class ConfirmChangesDialog : public KDialogBase
{
    TQ_OBJECT
public:
    ConfirmChangesDialog(const PkPackageList &changes, TQWidget *parent = nullptr);
};

#endif
