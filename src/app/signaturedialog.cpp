/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "signaturedialog.h"

#include <tqgrid.h>
#include <tqlabel.h>
#include <tqvbox.h>

#include <tdelocale.h>

#include "packageid.h"
#include "queryhelper.h"

SignatureDialog::SignatureDialog(const PkSignature &s, TQWidget *parent)
    : KDialogBase(parent, "signaturedialog", true, i18n("Trust This Software Source?"), Ok | Cancel, Cancel, true)
{
    /* Cancel is the default: trusting a key must be a deliberate act. */
    setButtonOK(KGuiItem(i18n("&Trust This Key"), "password"));

    TQVBox *page = makeVBoxMainWidget();
    TQLabel *intro = new TQLabel(
        i18n("A package is signed with a key that this computer does not trust yet. Only trust "
             "the key if you recognize the source and have checked its fingerprint. Software "
             "signed with a trusted key can make any change to this computer."),
        page);
    intro->setAlignment(TQt::WordBreak);

    TQGrid *grid = new TQGrid(2, page);
    grid->setSpacing(4);
    const auto add = [grid](const TQString &label, const TQString &value) {
        TQLabel *l = new TQLabel("<b>" + QueryHelper::escapeHtml(label) + "</b>", grid);
        l->setAlignment(TQt::AlignTop | TQt::AlignLeft);
        TQLabel *v = new TQLabel(value, grid);
        v->setTextFormat(TQt::PlainText);
        v->setAlignment(TQt::AlignTop | TQt::AlignLeft | TQt::WordBreak);
    };
    add(i18n("Package:"), PackageId(s.packageId).display());
    add(i18n("Repository:"), s.repository);
    add(i18n("Key owner:"), s.keyUserId);
    add(i18n("Key ID:"), s.keyId);
    add(i18n("Fingerprint:"), s.keyFingerprint);
    add(i18n("Created:"), s.keyTimestamp);
    add(i18n("Key address:"), s.keyUrl);
    resize(560, 320);
}

#include "signaturedialog.moc"
