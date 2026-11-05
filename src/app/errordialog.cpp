/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "errordialog.h"

#include <tqlabel.h>
#include <tqlayout.h>

#include <kiconloader.h>
#include <kpushbutton.h>
#include <ktextedit.h>
#include <tdeconfig.h>
#include <tdeglobal.h>
#include <tdeglobalsettings.h>
#include <tdelocale.h>
#include <tdemessagebox.h>

static const char ConfigGroup[] = "ErrorDialog";

void ErrorDialog::report(TQWidget *parent, const TQString &caption, const TQString &message,
                         const TQString &details, Kind kind)
{
    if (details.stripWhiteSpace().isEmpty()) {
        if (kind == Sorry) {
            KMessageBox::sorry(parent, message, caption);
        } else {
            KMessageBox::error(parent, message, caption);
        }
        return;
    }
    ErrorDialog dialog(parent, caption, message, details, kind);
    dialog.exec();
}

ErrorDialog::ErrorDialog(TQWidget *parent, const TQString &caption, const TQString &message,
                         const TQString &details, Kind kind)
    : KDialogBase(Plain, caption, Ok, Ok, parent, "errordialog", true, true)
{
    TQFrame *page = plainPage();
    TQVBoxLayout *top = new TQVBoxLayout(page, 0, spacingHint());

    TQHBoxLayout *messageRow = new TQHBoxLayout(top, spacingHint() * 2);
    TQLabel *icon = new TQLabel(page);
    icon->setPixmap(DesktopIcon(kind == Sorry ? "messagebox_warning" : "messagebox_critical",
                                TDEIcon::SizeMedium));
    icon->setAlignment(TQt::AlignTop);
    TQLabel *text = new TQLabel(message, page, "message");
    text->setAlignment(TQt::AlignTop | TQt::WordBreak);
    text->setMinimumWidth(300);
    messageRow->addWidget(icon, 0, TQt::AlignTop);
    messageRow->addWidget(text, 1);

    TQHBoxLayout *buttonRow = new TQHBoxLayout(top);
    m_toggle = new KPushButton(page, "detailsbutton");
    buttonRow->addWidget(m_toggle);
    buttonRow->addStretch(1);

    m_details = new KTextEdit(page, "details");
    m_details->setReadOnly(true);
    m_details->setTextFormat(TQt::PlainText);
    m_details->setWordWrap(TQTextEdit::NoWrap);
    m_details->setFont(TDEGlobalSettings::fixedFont());
    m_details->setText(details);
    m_details->setMinimumHeight(80);
    top->addWidget(m_details, 1);

    connect(m_toggle, TQ_SIGNAL(clicked()), TQ_SLOT(toggleDetails()));

    /* The details start open, so the size the user chose last time is restored. */
    m_toggle->setText(i18n("&Details <<"));
    resize(configDialogSize(ConfigGroup).expandedTo(TQSize(560, 360)));
}

ErrorDialog::~ErrorDialog()
{
    if (m_details->isVisible()) {
        saveDialogSize(ConfigGroup);
        TDEGlobal::config()->sync();
    }
}

void ErrorDialog::toggleDetails()
{
    if (m_details->isVisible()) {
        m_details->hide();
        m_toggle->setText(i18n("&Details >>"));
        /* Shrink to fit, but stay resizable: no fixed size is ever set. */
        resize(width(), minimumSizeHint().height());
    } else {
        m_details->show();
        m_toggle->setText(i18n("&Details <<"));
        resize(width(), TQMAX(height(), 360));
    }
}

#include "errordialog.moc"
