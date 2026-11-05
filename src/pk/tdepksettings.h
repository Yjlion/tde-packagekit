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

    /* Seconds since the epoch of the notifier's last automatic check. */
    static long lastAutomaticCheck();
    static void setLastAutomaticCheck(long when);

    /* Seconds the notifier waits after login before its first check. */
    static int initialDelaySeconds();

    static void sync();
};

#endif
