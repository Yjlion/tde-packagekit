/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <tqfileinfo.h>
#include <tqdir.h>

#include <kuniqueapplication.h>
#include <tdeaboutdata.h>
#include <tdecmdlineargs.h>
#include <tdelocale.h>
#include <tdemessagebox.h>

#include "mainwindow.h"

static const char description[] = I18N_NOOP("Install, remove and update software with PackageKit");

static TDECmdLineOptions options[] = {
    { "updates", I18N_NOOP("Open the Updates page"), 0 },
    { "settings", I18N_NOOP("Open the settings dialog"), 0 },
    { "install-file <file>", I18N_NOOP("Install a package file"), 0 },
    TDECmdLineLastOption
};

class App : public KUniqueApplication
{
public:
    App() : m_window(nullptr) {}

    int newInstance() override
    {
        if (!m_window) {
            m_window = new MainWindow();
            setMainWidget(m_window);
        }
        m_window->show();
        m_window->raise();

        TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();
        if (args->isSet("updates")) {
            m_window->showPage(MainWindow::UpdatesPageIndex);
        }
        if (args->isSet("settings")) {
            m_window->showSettings();
        }
        const QCStringList files = args->getOptionList("install-file");
        if (!files.isEmpty()) {
            TQStringList paths;
            const TQDir cwd(TDECmdLineArgs::cwd());
            for (QCStringList::const_iterator it = files.begin(); it != files.end(); ++it) {
                paths << TQFileInfo(cwd, TQString::fromLocal8Bit(*it)).absFilePath();
            }
            m_window->installFiles(paths);
        }
        args->clear();
        return 0;
    }

private:
    MainWindow *m_window;
};

int main(int argc, char **argv)
{
    TDEAboutData about("tdepackagekit", I18N_NOOP("Software Management"), TDEPK_VERSION,
                       description, TDEAboutData::License_GPL_V2,
                       "(c) 2026 Yjlion");
    about.addAuthor("Yjlion", I18N_NOOP("Maintainer"), 0, "https://github.com/Yjlion");
    TDECmdLineArgs::init(argc, argv, &about);
    TDECmdLineArgs::addCmdLineOptions(options);
    KUniqueApplication::addCmdLineOptions();

    if (!KUniqueApplication::start()) {
        return 0;
    }
    App app;
    return app.exec();
}
