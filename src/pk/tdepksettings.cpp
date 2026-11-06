/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "tdepksettings.h"

#include <tdeconfig.h>
#include <tqstring.h>

static TDEConfig *config()
{
    static TDEConfig *instance = new TDEConfig("tdepackagekitrc");
    instance->reparseConfiguration();
    instance->setGroup("Notifier");
    return instance;
}

int TdePkSettings::checkIntervalHours() { return config()->readNumEntry("CheckIntervalHours", 24); }
void TdePkSettings::setCheckIntervalHours(int hours) { config()->writeEntry("CheckIntervalHours", hours); sync(); }

bool TdePkSettings::notifySecurityOnly() { return config()->readBoolEntry("NotifySecurityOnly", false); }
void TdePkSettings::setNotifySecurityOnly(bool only) { config()->writeEntry("NotifySecurityOnly", only); sync(); }

bool TdePkSettings::notifierEnabled() { return config()->readBoolEntry("Enabled", true); }
void TdePkSettings::setNotifierEnabled(bool enabled) { config()->writeEntry("Enabled", enabled); sync(); }

bool TdePkSettings::hideWhenIdle() { return config()->readBoolEntry("HideWhenIdle", false); }
void TdePkSettings::setHideWhenIdle(bool hide) { config()->writeEntry("HideWhenIdle", hide); sync(); }

long TdePkSettings::lastAutomaticCheck() { return config()->readEntry("LastCheck", "0").toLong(); }
void TdePkSettings::setLastAutomaticCheck(long when) { config()->writeEntry("LastCheck", TQString::number(when)); sync(); }

int TdePkSettings::initialDelaySeconds() { return config()->readNumEntry("InitialDelaySeconds", 120); }

void TdePkSettings::sync() { config()->sync(); }
