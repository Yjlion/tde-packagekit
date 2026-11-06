/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <kuniqueapplication.h>
#include <tdeaboutdata.h>
#include <tdecmdlineargs.h>
#include <tdelocale.h>

#include "notifier.h"
#include "tdepksettings.h"

int main(int argc, char **argv)
{
    TDEAboutData about("tdepackagekit-notifier", I18N_NOOP("Software Update Notifier"), TDEPK_VERSION,
                       I18N_NOOP("Shows when software updates are available"),
                       TDEAboutData::License_GPL_V2, "(c) 2026 Yjlion");
    about.addAuthor("Yjlion", I18N_NOOP("Maintainer"), 0, "https://github.com/Yjlion");
    /* No "Please use bugs.trinitydesktop.org" line: this isn't a Trinity project. */
    about.setCustomAuthorText(TQString::null, TQString::null);
    TDECmdLineArgs::init(argc, argv, &about);
    KUniqueApplication::addCmdLineOptions();

    if (!KUniqueApplication::start()) {
        return 0;
    }
    KUniqueApplication app;
    app.disableSessionManagement();
    if (!TdePkSettings::notifierEnabled()) {
        return 0;
    }
    Notifier *notifier = new Notifier();
    app.setMainWidget(notifier);
    return app.exec();
}
