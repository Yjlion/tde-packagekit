/*
 * Settings shared by the application and the notifier (tdepackagekitrc).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef TDEPK_SETTINGS_H
#define TDEPK_SETTINGS_H

class TdePkSettings
{
public:
    /* Hours between update checks. 0 means never check automatically. */
    static int checkIntervalHours();
    static void setCheckIntervalHours(int hours);

    static bool notifySecurityOnly();
    static void setNotifySecurityOnly(bool only);

    static bool notifierEnabled();
    static void setNotifierEnabled(bool enabled);

    /* Hide the tray icon while there are no updates. Off by default, so the
       icon shows from login and the user can see the notifier is running. */
    static bool hideWhenIdle();
    static void setHideWhenIdle(bool hide);

    /* Seconds since the epoch of the notifier's last automatic check. */
    static long lastAutomaticCheck();
    static void setLastAutomaticCheck(long when);

    /* Seconds the notifier waits after login before its first check. */
    static int initialDelaySeconds();

    static void sync();
};

#endif
