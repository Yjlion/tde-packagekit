/*
 * Error message with technical details, in a window that can be resized.
 *
 * KMessageBox::detailedError shows its details as a TQDialog extension, and
 * TQt fixes the dialog's size while an extension is shown, so long details
 * can't be read. This dialog shows them in a text area that grows with the window.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef TDEPK_ERRORDIALOG_H
#define TDEPK_ERRORDIALOG_H

#include <kdialogbase.h>

class KPushButton;
class KTextEdit;

class ErrorDialog : public KDialogBase
{
    TQ_OBJECT
public:
    enum Kind { Error, Sorry };

    /* Shows the message and waits until it is closed. Without details this is
       a plain KMessageBox. */
    static void report(TQWidget *parent, const TQString &caption, const TQString &message,
                       const TQString &details, Kind kind = Error);

protected:
    ErrorDialog(TQWidget *parent, const TQString &caption, const TQString &message,
                const TQString &details, Kind kind);
    ~ErrorDialog();

private slots:
    void toggleDetails();

private:
    KPushButton *m_toggle;
    KTextEdit *m_details;
};

#endif
