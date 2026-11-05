/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "pkenums.h"

#include <tdelocale.h>

namespace Pk {

TQString roleLabel(uint role)
{
    switch (role) {
    case RoleCancel:            return i18n("Cancelling");
    case RoleDependsOn:         return i18n("Getting dependencies");
    case RoleGetDetails:        return i18n("Getting details");
    case RoleGetFiles:          return i18n("Getting file list");
    case RoleGetPackages:       return i18n("Getting package list");
    case RoleGetRepoList:       return i18n("Getting repositories");
    case RoleRequiredBy:        return i18n("Getting requirements");
    case RoleGetUpdateDetail:   return i18n("Getting update details");
    case RoleGetUpdates:        return i18n("Checking for updates");
    case RoleInstallFiles:      return i18n("Installing files");
    case RoleInstallPackages:   return i18n("Installing packages");
    case RoleInstallSignature:  return i18n("Installing signature");
    case RoleRefreshCache:      return i18n("Refreshing package information");
    case RoleRemovePackages:    return i18n("Removing packages");
    case RoleRepoEnable:        return i18n("Changing repository");
    case RoleResolve:           return i18n("Finding packages");
    case RoleSearchDetails:
    case RoleSearchFile:
    case RoleSearchGroup:
    case RoleSearchName:        return i18n("Searching");
    case RoleUpdatePackages:    return i18n("Updating packages");
    case RoleAcceptEula:        return i18n("Accepting license");
    case RoleDownloadPackages:  return i18n("Downloading packages");
    case RoleGetOldTransactions:return i18n("Getting history");
    case RoleUpgradeSystem:     return i18n("Upgrading the system");
    case RoleRepairSystem:      return i18n("Repairing the system");
    default:                    return i18n("Working");
    }
}

TQString roleLabelPast(uint role)
{
    switch (role) {
    case RoleInstallFiles:      return i18n("Installed files");
    case RoleInstallPackages:   return i18n("Installed packages");
    case RoleInstallSignature:  return i18n("Installed signature");
    case RoleRefreshCache:      return i18n("Refreshed package information");
    case RoleRemovePackages:    return i18n("Removed packages");
    case RoleRepoEnable:        return i18n("Changed repository");
    case RoleUpdatePackages:    return i18n("Updated packages");
    case RoleAcceptEula:        return i18n("Accepted license");
    case RoleDownloadPackages:  return i18n("Downloaded packages");
    case RoleUpgradeSystem:     return i18n("Upgraded the system");
    case RoleRepairSystem:      return i18n("Repaired the system");
    case RoleGetUpdates:        return i18n("Checked for updates");
    default:                    return roleLabel(role);
    }
}

TQString statusLabel(uint status)
{
    switch (status) {
    case StatusWait:                return i18n("Waiting in queue");
    case StatusSetup:               return i18n("Setting up");
    case StatusRunning:             return i18n("Running");
    case StatusQuery:               return i18n("Querying");
    case StatusInfo:                return i18n("Getting information");
    case StatusRemove:              return i18n("Removing");
    case StatusRefreshCache:        return i18n("Refreshing package information");
    case StatusDownload:            return i18n("Downloading");
    case StatusInstall:             return i18n("Installing");
    case StatusUpdate:              return i18n("Updating");
    case StatusCleanup:             return i18n("Cleaning up");
    case StatusObsolete:            return i18n("Obsoleting");
    case StatusDepResolve:          return i18n("Resolving dependencies");
    case StatusSigCheck:            return i18n("Checking signatures");
    case StatusTestCommit:          return i18n("Testing changes");
    case StatusCommit:              return i18n("Committing changes");
    case StatusRequest:             return i18n("Requesting data");
    case StatusFinished:            return i18n("Finished");
    case StatusCancel:              return i18n("Cancelling");
    case StatusDownloadRepository:  return i18n("Downloading repository information");
    case StatusDownloadPackagelist: return i18n("Downloading package list");
    case StatusDownloadFilelist:    return i18n("Downloading file lists");
    case StatusDownloadChangelog:   return i18n("Downloading changelog");
    case StatusDownloadGroup:       return i18n("Downloading groups");
    case StatusDownloadUpdateinfo:  return i18n("Downloading update information");
    case StatusRepackaging:         return i18n("Repackaging files");
    case StatusLoadingCache:        return i18n("Loading cache");
    case StatusScanApplications:    return i18n("Scanning applications");
    case StatusGeneratePackageList: return i18n("Generating package lists");
    case StatusWaitingForLock:      return i18n("Waiting for package manager lock");
    case StatusWaitingForAuth:      return i18n("Waiting for authentication");
    case StatusScanProcessList:     return i18n("Checking running applications");
    case StatusCheckExecutableFiles:return i18n("Checking applications in use");
    case StatusCheckLibraries:      return i18n("Checking libraries in use");
    case StatusCopyFiles:           return i18n("Copying files");
    case StatusRunHook:             return i18n("Running hooks");
    default:                        return i18n("Working");
    }
}

TQString infoLabel(uint info)
{
    switch (info) {
    case InfoInstalled:     return i18n("Installed");
    case InfoAvailable:     return i18n("Available");
    case InfoLow:           return i18n("Minor update");
    case InfoEnhancement:   return i18n("Enhancement");
    case InfoNormal:        return i18n("Normal update");
    case InfoBugfix:        return i18n("Bug fix");
    case InfoImportant:     return i18n("Important update");
    case InfoSecurity:      return i18n("Security update");
    case InfoCritical:      return i18n("Critical update");
    case InfoBlocked:       return i18n("Blocked update");
    case InfoDownloading:   return i18n("Downloading");
    case InfoUpdating:      return i18n("Updating");
    case InfoInstalling:    return i18n("Installing");
    case InfoRemoving:      return i18n("Removing");
    case InfoCleanup:       return i18n("Cleaning up");
    case InfoObsoleting:    return i18n("Obsoleting");
    case InfoFinished:      return i18n("Finished");
    case InfoReinstalling:  return i18n("Reinstalling");
    case InfoDowngrading:   return i18n("Downgrading");
    case InfoPreparing:     return i18n("Preparing");
    case InfoDecompressing: return i18n("Decompressing");
    case InfoUntrusted:     return i18n("Untrusted");
    case InfoTrusted:       return i18n("Trusted");
    case InfoUnavailable:   return i18n("Unavailable");
    case InfoInstall:       return i18n("To be installed");
    case InfoRemove:        return i18n("To be removed");
    case InfoObsolete:      return i18n("To be obsoleted");
    case InfoDowngrade:     return i18n("To be downgraded");
    default:                return i18n("Unknown");
    }
}

TQString errorLabel(uint code)
{
    switch (code) {
    case ErrorOom:                       return i18n("The system ran out of memory.");
    case ErrorNoNetwork:                 return i18n("No network connection is available.");
    case ErrorNotSupported:              return i18n("The package system does not support this action.");
    case ErrorInternalError:             return i18n("The package system reported an internal error.");
    case ErrorGpgFailure:                return i18n("A package signature could not be verified.");
    case ErrorPackageNotInstalled:       return i18n("The package is not installed.");
    case ErrorPackageNotFound:           return i18n("The package could not be found.");
    case ErrorPackageAlreadyInstalled:   return i18n("The package is already installed.");
    case ErrorPackageDownloadFailed:     return i18n("A package could not be downloaded.");
    case ErrorDepResolutionFailed:       return i18n("The package dependencies could not be resolved.");
    case ErrorTransactionError:          return i18n("The package operation failed.");
    case ErrorTransactionCancelled:      return i18n("The package operation was cancelled.");
    case ErrorNoCache:                   return i18n("Package information has not been downloaded yet. Refresh it and try again.");
    case ErrorRepoNotFound:              return i18n("The repository could not be found.");
    case ErrorCannotRemoveSystemPackage: return i18n("This package is required by the system and cannot be removed.");
    case ErrorCannotCancel:              return i18n("This operation can no longer be cancelled.");
    case ErrorCannotGetLock:             return i18n("Another program is using the package manager. Close it and try again.");
    case ErrorNoPackagesToUpdate:        return i18n("There are no packages to update.");
    case ErrorCannotWriteRepoConfig:     return i18n("The repository configuration could not be saved.");
    case ErrorLocalInstallFailed:        return i18n("The package file could not be installed.");
    case ErrorBadGpgSignature:           return i18n("A package has a bad signature.");
    case ErrorMissingGpgSignature:       return i18n("A package has no signature.");
    case ErrorRepoConfigurationError:    return i18n("A repository is configured incorrectly.");
    case ErrorNoLicenseAgreement:        return i18n("A license agreement was not accepted.");
    case ErrorFileConflicts:             return i18n("Packages have conflicting files.");
    case ErrorPackageConflicts:          return i18n("Packages conflict with each other.");
    case ErrorRepoNotAvailable:          return i18n("A repository is not available.");
    case ErrorInvalidPackageFile:        return i18n("The file is not a valid package.");
    case ErrorPackageInstallBlocked:     return i18n("Installing this package is blocked.");
    case ErrorPackageCorrupt:            return i18n("A package is corrupt.");
    case ErrorAllPackagesAlreadyInstalled: return i18n("All of the packages are already installed.");
    case ErrorFileNotFound:              return i18n("The file could not be found.");
    case ErrorNoMoreMirrorsToTry:        return i18n("No more download mirrors are available.");
    case ErrorIncompatibleArchitecture:  return i18n("The package is not compatible with this system.");
    case ErrorNoSpaceOnDevice:           return i18n("There is not enough free disk space.");
    case ErrorMediaChangeRequired:       return i18n("Another installation medium is required.");
    case ErrorNotAuthorized:             return i18n("You are not authorized to do this. Check that the TDE authentication agent is running, then try again.");
    case ErrorUpdateNotFound:            return i18n("The update could not be found.");
    case ErrorCannotInstallRepoUnsigned: return i18n("Packages from an unsigned repository cannot be installed.");
    case ErrorCannotUpdateRepoUnsigned:  return i18n("Packages from an unsigned repository cannot be updated.");
    case ErrorCannotDisableRepository:   return i18n("This repository cannot be disabled.");
    case ErrorPackageFailedToConfigure:  return i18n("A package failed to configure.");
    case ErrorPackageFailedToInstall:    return i18n("A package failed to install.");
    case ErrorPackageFailedToRemove:     return i18n("A package failed to remove.");
    case ErrorUpdateFailedDueToRunningProcess: return i18n("An update failed because the program is running.");
    case ErrorPackageDatabaseChanged:    return i18n("The package database changed during the operation. Try again.");
    case ErrorCannotFetchSources:        return i18n("Package sources could not be fetched.");
    case ErrorUnfinishedTransaction:     return i18n("An earlier package operation did not finish.");
    default:                             return i18n("The package system reported an error (code %1).").arg(code);
    }
}

TQString groupLabel(uint group)
{
    switch (group) {
    case GroupAccessibility:   return i18n("Accessibility");
    case GroupAccessories:     return i18n("Accessories");
    case GroupAdminTools:      return i18n("Administration");
    case GroupCommunication:   return i18n("Communication");
    case GroupDesktopGnome:    return i18n("GNOME Desktop");
    case GroupDesktopKde:      return i18n("KDE Desktop");
    case GroupDesktopOther:    return i18n("Other Desktops");
    case GroupDesktopXfce:     return i18n("Xfce Desktop");
    case GroupEducation:       return i18n("Education");
    case GroupFonts:           return i18n("Fonts");
    case GroupGames:           return i18n("Games");
    case GroupGraphics:        return i18n("Graphics");
    case GroupInternet:        return i18n("Internet");
    case GroupLegacy:          return i18n("Legacy");
    case GroupLocalization:    return i18n("Localization");
    case GroupMaps:            return i18n("Maps");
    case GroupMultimedia:      return i18n("Multimedia");
    case GroupNetwork:         return i18n("Network");
    case GroupOffice:          return i18n("Office");
    case GroupOther:           return i18n("Other");
    case GroupPowerManagement: return i18n("Power Management");
    case GroupProgramming:     return i18n("Programming");
    case GroupPublishing:      return i18n("Publishing");
    case GroupRepos:           return i18n("Repositories");
    case GroupSecurity:        return i18n("Security");
    case GroupServers:         return i18n("Servers");
    case GroupSystem:          return i18n("System");
    case GroupVirtualization:  return i18n("Virtualization");
    case GroupScience:         return i18n("Science");
    case GroupDocumentation:   return i18n("Documentation");
    case GroupElectronics:     return i18n("Electronics");
    case GroupCollections:     return i18n("Collections");
    case GroupVendor:          return i18n("Vendor");
    case GroupNewest:          return i18n("Newest");
    case GroupDesktopDde:      return i18n("Deepin Desktop");
    default:                   return i18n("Unknown");
    }
}

TQString groupId(uint group)
{
    switch (group) {
    case GroupAccessibility:   return "accessibility";
    case GroupAccessories:     return "accessories";
    case GroupAdminTools:      return "admin-tools";
    case GroupCommunication:   return "communication";
    case GroupDesktopGnome:    return "desktop-gnome";
    case GroupDesktopKde:      return "desktop-kde";
    case GroupDesktopOther:    return "desktop-other";
    case GroupDesktopXfce:     return "desktop-xfce";
    case GroupEducation:       return "education";
    case GroupFonts:           return "fonts";
    case GroupGames:           return "games";
    case GroupGraphics:        return "graphics";
    case GroupInternet:        return "internet";
    case GroupLegacy:          return "legacy";
    case GroupLocalization:    return "localization";
    case GroupMaps:            return "maps";
    case GroupMultimedia:      return "multimedia";
    case GroupNetwork:         return "network";
    case GroupOffice:          return "office";
    case GroupOther:           return "other";
    case GroupPowerManagement: return "power-management";
    case GroupProgramming:     return "programming";
    case GroupPublishing:      return "publishing";
    case GroupRepos:           return "repos";
    case GroupSecurity:        return "security";
    case GroupServers:         return "servers";
    case GroupSystem:          return "system";
    case GroupVirtualization:  return "virtualization";
    case GroupScience:         return "science";
    case GroupDocumentation:   return "documentation";
    case GroupElectronics:     return "electronics";
    case GroupCollections:     return "collections";
    case GroupVendor:          return "vendor";
    case GroupNewest:          return "newest";
    case GroupDesktopDde:      return "desktop-dde";
    default:                   return "unknown";
    }
}

TQString groupIcon(uint group)
{
    /* Names from the Trinity crystalsvg theme, or the freedesktop name that fuller
       icon themes provide. The caller falls back to a folder for missing icons. */
    switch (group) {
    case GroupAccessibility:   return "preferences-desktop-accessibility";
    case GroupAccessories:     return "accessories-calculator";
    case GroupAdminTools:      return "utilities-system-monitor";
    case GroupCommunication:   return "internet-mail";
    case GroupDesktopGnome:
    case GroupDesktopKde:
    case GroupDesktopOther:
    case GroupDesktopXfce:
    case GroupDesktopDde:      return "desktop";
    case GroupEducation:       return "accessories-dictionary";
    case GroupFonts:           return "preferences-desktop-font";
    case GroupGames:           return "applications-games";
    case GroupGraphics:        return "applications-graphics";
    case GroupInternet:        return "applications-internet";
    case GroupLocalization:    return "preferences-desktop-locale";
    case GroupMaps:            return "applications-internet";
    case GroupMultimedia:      return "preferences-desktop-multimedia";
    case GroupNetwork:         return "network_local";
    case GroupOffice:          return "x-office-document";
    case GroupPowerManagement: return "system-shutdown";
    case GroupProgramming:     return "utilities-terminal";
    case GroupPublishing:      return "print_class";
    case GroupRepos:           return "folder_green";
    case GroupSecurity:        return "password";
    case GroupServers:         return "network_local";
    case GroupSystem:          return "utilities-system-monitor";
    case GroupVirtualization:  return "system-run";
    case GroupScience:         return "applications-science";
    case GroupDocumentation:   return "help";
    case GroupElectronics:     return "memory";
    case GroupCollections:     return "system-software-install";
    case GroupVendor:          return "system-software-install";
    case GroupNewest:          return "bookmark";
    default:                   return "folder";
    }
}

TQString restartLabel(uint restart)
{
    switch (restart) {
    case RestartApplication:     return i18n("Restart the application to finish the update.");
    case RestartSession:         return i18n("Log out and back in to finish the update.");
    case RestartSystem:          return i18n("Restart the computer to finish the update.");
    case RestartSecuritySession: return i18n("Log out and back in to finish this important security update.");
    case RestartSecuritySystem:  return i18n("Restart the computer to finish this important security update.");
    default:                     return TQString();
    }
}

int updateSeverityRank(uint info)
{
    switch (info) {
    case InfoCritical:    return 0;
    case InfoSecurity:    return 1;
    case InfoImportant:   return 2;
    case InfoBugfix:      return 3;
    case InfoNormal:      return 4;
    case InfoUnknown:     return 4;
    case InfoEnhancement: return 5;
    case InfoLow:         return 6;
    case InfoBlocked:     return 7;
    default:              return 4;
    }
}

bool infoIsInstalled(uint info)
{
    return info == InfoInstalled || info == InfoCollectionInstalled;
}

} // namespace Pk
