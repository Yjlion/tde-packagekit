/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "euladialog.h"

#include <tqcheckbox.h>
#include <tqlabel.h>
#include <tqvbox.h>

#include <ktextbrowser.h>
#include <tdelocale.h>

#include "packageid.h"

EulaDialog::EulaDialog(const PkEula &eula, TQWidget *parent)
    : KDialogBase(parent, "euladialog", true, i18n("License Agreement"), Ok | Cancel, Cancel, true)
{
    setButtonOK(KGuiItem(i18n("&Continue"), "ok"));
    enableButtonOK(false);

    TQVBox *page = makeVBoxMainWidget();
    TQLabel *intro = new TQLabel(page);
    intro->setTextFormat(TQt::PlainText);
    intro->setAlignment(TQt::WordBreak);
    intro->setText(i18n("%1 from %2 has a license agreement that must be accepted before it can be installed.")
                       .arg(PackageId(eula.packageId).name()).arg(eula.vendor));

    KTextBrowser *text = new KTextBrowser(page, "eulatext");
    text->setTextFormat(TQt::PlainText);
    text->setText(eula.agreement);

    TQCheckBox *accept = new TQCheckBox(i18n("I &accept the terms of this agreement"), page, "eulaaccept");
    connect(accept, TQ_SIGNAL(toggled(bool)), TQ_SLOT(acceptToggled(bool)));
    resize(600, 440);
}

void EulaDialog::acceptToggled(bool on)
{
    enableButtonOK(on);
}

#include "euladialog.moc"
