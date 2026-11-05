/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef TDEPK_SIGNATUREDIALOG_H
#define TDEPK_SIGNATUREDIALOG_H

#include <kdialogbase.h>

#include "pktypes.h"

class SignatureDialog : public KDialogBase
{
    TQ_OBJECT
public:
    SignatureDialog(const PkSignature &signature, TQWidget *parent = nullptr);
};

#endif
