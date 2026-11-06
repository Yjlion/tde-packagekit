/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "notifier.h"

#include <time.h>

#include <tqtimer.h>
#include <tqtooltip.h>

#include <kiconloader.h>
#include <kpassivepopup.h>
#include <krun.h>
#include <tdeapplication.h>
#include <tdelocale.h>
#include <tdepopupmenu.h>

#include "pkdaemon.h"
#include "pkenums.h"
#include "pktransaction.h"
#include "tdepksettings.h"

/* How often the notifier looks at whether a check is due. */
static const int TickMilliseconds = 30 * 60 * 1000;

Notifier::Notifier()
    : KSystemTray(nullptr, "notifier"), DCOPObject("notifier"),
      m_refresh(nullptr), m_updates(nullptr),
      m_total(0), m_security(0), m_lastNotified(0), m_pendingList(false), m_known(false)
{
    setPixmap(loadIcon("tdepackagekit"));
    TQToolTip::add(this, i18n("Software Updates: not checked yet"));
    if (!TdePkSettings::hideWhenIdle()) {
        show();
    }

    m_timer = new TQTimer(this);
    connect(m_timer, TQ_SIGNAL(timeout()), TQ_SLOT(onTick()));
    m_timer->start(TickMilliseconds);
    TQTimer::singleShot(TdePkSettings::initialDelaySeconds() * 1000, this, TQ_SLOT(onTick()));

    connect(PkDaemon::self(), TQ_SIGNAL(updatesChanged()), TQ_SLOT(onUpdatesChanged()));
}

bool Notifier::process(const TQCString &fun, const TQByteArray &data,
                       TQCString &replyType, TQByteArray &replyData)
{
    if (fun == "reloadSettings()") {
        replyType = "void";
        reloadSettings();
        return true;
    }
    if (fun == "checkNow()") {
        replyType = "void";
        checkNow();
        return true;
    }
    return DCOPObject::process(fun, data, replyType, replyData);
}

QCStringList Notifier::functions()
{
    QCStringList list = DCOPObject::functions();
    list << "void reloadSettings()" << "void checkNow()";
    return list;
}

void Notifier::contextMenuAboutToShow(TDEPopupMenu *menu)
{
    menu->clear();
    menu->insertTitle(loadIcon("tdepackagekit"), i18n("Software Updates"));
    menu->insertItem(SmallIcon("system-software-update"), i18n("&Show Updates"), this, TQ_SLOT(openUpdates()));
    menu->insertItem(SmallIcon("reload"), i18n("&Check Now"), this, TQ_SLOT(checkNow()));
    menu->insertItem(SmallIcon("configure"), i18n("C&onfigure..."), this, TQ_SLOT(openSettings()));
    menu->insertSeparator();
    menu->insertItem(SmallIcon("exit"), i18n("&Quit"), kapp, TQ_SLOT(quit()));
}

void Notifier::mousePressEvent(TQMouseEvent *event)
{
    if (event->button() == TQt::LeftButton) {
        openUpdates();
        return;
    }
    KSystemTray::mousePressEvent(event);
}

void Notifier::openUpdates()
{
    KRun::runCommand("tdepackagekit --updates");
}

void Notifier::openSettings()
{
    KRun::runCommand("tdepackagekit --settings");
}

void Notifier::reloadSettings()
{
    if (!TdePkSettings::notifierEnabled()) {
        kapp->quit();
        return;
    }
    showState();
    onTick();
}

void Notifier::onTick()
{
    if (!TdePkSettings::notifierEnabled()) {
        kapp->quit();
        return;
    }
    const int hours = TdePkSettings::checkIntervalHours();
    const long now = time(nullptr);
    if (hours > 0 && now - TdePkSettings::lastAutomaticCheck() >= hours * 3600L) {
        checkNow();
    } else if (!m_updates) {
        /* Between checks the count can still change, for example after updating
           with another tool. Looking at it needs no network. */
        listUpdates();
    }
}

void Notifier::checkNow()
{
    if (m_refresh) {
        return;
    }
    TdePkSettings::setLastAutomaticCheck(time(nullptr));
    m_refresh = new PkTransaction(this);
    connect(m_refresh, TQ_SIGNAL(finished(uint, uint)), TQ_SLOT(onRefreshFinished(uint, uint)));
    m_refresh->refreshCache(false);
}

void Notifier::onRefreshFinished(uint, uint)
{
    m_refresh->deleteLater();
    m_refresh = nullptr;
    /* Refreshing can fail, for example offline. The list from the last refresh still applies. */
    listUpdates();
}

void Notifier::onUpdatesChanged()
{
    listUpdates();
}

void Notifier::listUpdates()
{
    if (m_updates) {
        m_pendingList = true;
        return;
    }
    m_total = 0;
    m_security = 0;
    m_updates = new PkTransaction(this);
    connect(m_updates, TQ_SIGNAL(package(const PkPackage &)), TQ_SLOT(onPackage(const PkPackage &)));
    connect(m_updates, TQ_SIGNAL(finished(uint, uint)), TQ_SLOT(onUpdatesFinished(uint, uint)));
    m_updates->getUpdates(Pk::bit(Pk::FilterNone));
}

void Notifier::onPackage(const PkPackage &package)
{
    if (package.info == Pk::InfoBlocked) {
        return;
    }
    ++m_total;
    if (package.info == Pk::InfoSecurity || package.info == Pk::InfoCritical) {
        ++m_security;
    }
}

void Notifier::onUpdatesFinished(uint exit, uint)
{
    m_updates->deleteLater();
    m_updates = nullptr;
    if (exit == Pk::ExitSuccess) {
        m_known = true;
        showState();
    }
    if (m_pendingList) {
        m_pendingList = false;
        listUpdates();
    }
}

void Notifier::showState()
{
    const bool securityOnly = TdePkSettings::notifySecurityOnly();
    const uint relevant = securityOnly ? m_security : m_total;
    if (relevant == 0) {
        m_lastNotified = 0;
        TQToolTip::remove(this);
        TQToolTip::add(this, !m_known ? i18n("Software Updates: not checked yet")
                             : m_total > 0 ? i18n("No security updates")
                                           : i18n("Your software is up to date"));
        setPixmap(loadIcon("tdepackagekit"));
        if (TdePkSettings::hideWhenIdle()) {
            hide();
        } else {
            show();
        }
        return;
    }

    const TQString text = m_security > 0
        ? i18n("%1 of the available updates fix security problems.")
              .arg(m_security)
        : i18n("Updates are available for this computer.");
    const TQString title = i18n("1 update available", "%n updates available", m_total);
    TQToolTip::remove(this);
    TQToolTip::add(this, title);
    setPixmap(loadIcon(m_security > 0 ? "messagebox_warning" : "tdepackagekit"));
    show();

    /* Only speak up when there is something new since the last popup. */
    if (relevant > m_lastNotified) {
        KPassivePopup::message(title, text, loadIcon("tdepackagekit"), this);
    }
    m_lastNotified = relevant;
}

#include "notifier.moc"
