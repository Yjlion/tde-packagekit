/*
 * PackageKit enumerations, as used on its D-Bus interface.
 *
 * The numeric values are part of the PackageKit D-Bus API. tests/unit checks
 * them against <packagekit-glib2/pk-enum.h> so that drift is caught at test
 * time, without this library having to link against libpackagekit-glib2.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef TDEPK_PKENUMS_H
#define TDEPK_PKENUMS_H

#include <tqglobal.h>
#include <tqstring.h>

namespace Pk {

typedef TQ_UINT64 Bitfield;

inline Bitfield bit(uint value) { return Bitfield(1) << value; }

enum Role {
    RoleUnknown, RoleCancel, RoleDependsOn, RoleGetDetails, RoleGetFiles,
    RoleGetPackages, RoleGetRepoList, RoleRequiredBy, RoleGetUpdateDetail,
    RoleGetUpdates, RoleInstallFiles, RoleInstallPackages, RoleInstallSignature,
    RoleRefreshCache, RoleRemovePackages, RoleRepoEnable, RoleRepoSetData,
    RoleResolve, RoleSearchDetails, RoleSearchFile, RoleSearchGroup,
    RoleSearchName, RoleUpdatePackages, RoleWhatProvides, RoleAcceptEula,
    RoleDownloadPackages, RoleGetDistroUpgrades, RoleGetCategories,
    RoleGetOldTransactions, RoleRepairSystem, RoleGetDetailsLocal,
    RoleGetFilesLocal, RoleRepoRemove, RoleUpgradeSystem
};

enum Status {
    StatusUnknown, StatusWait, StatusSetup, StatusRunning, StatusQuery,
    StatusInfo, StatusRemove, StatusRefreshCache, StatusDownload,
    StatusInstall, StatusUpdate, StatusCleanup, StatusObsolete,
    StatusDepResolve, StatusSigCheck, StatusTestCommit, StatusCommit,
    StatusRequest, StatusFinished, StatusCancel, StatusDownloadRepository,
    StatusDownloadPackagelist, StatusDownloadFilelist, StatusDownloadChangelog,
    StatusDownloadGroup, StatusDownloadUpdateinfo, StatusRepackaging,
    StatusLoadingCache, StatusScanApplications, StatusGeneratePackageList,
    StatusWaitingForLock, StatusWaitingForAuth, StatusScanProcessList,
    StatusCheckExecutableFiles, StatusCheckLibraries, StatusCopyFiles,
    StatusRunHook
};

enum Exit {
    ExitUnknown, ExitSuccess, ExitFailed, ExitCancelled, ExitKeyRequired,
    ExitEulaRequired, ExitKilled, ExitMediaChangeRequired, ExitNeedUntrusted,
    ExitCancelledPriority, ExitSkipTransaction, ExitRepairRequired
};

enum Network {
    NetworkUnknown, NetworkOffline, NetworkOnline, NetworkWired, NetworkWifi,
    NetworkMobile
};

enum Filter {
    FilterUnknown, FilterNone, FilterInstalled, FilterNotInstalled,
    FilterDevelopment, FilterNotDevelopment, FilterGui, FilterNotGui,
    FilterFree, FilterNotFree, FilterVisible, FilterNotVisible,
    FilterSupported, FilterNotSupported, FilterBasename, FilterNotBasename,
    FilterNewest, FilterNotNewest, FilterArch, FilterNotArch, FilterSource,
    FilterNotSource, FilterCollections, FilterNotCollections,
    FilterApplication, FilterNotApplication, FilterDownloaded,
    FilterNotDownloaded
};

enum Restart {
    RestartUnknown, RestartNone, RestartApplication, RestartSession,
    RestartSystem, RestartSecuritySession, RestartSecuritySystem
};

enum Error {
    ErrorUnknown, ErrorOom, ErrorNoNetwork, ErrorNotSupported,
    ErrorInternalError, ErrorGpgFailure, ErrorPackageIdInvalid,
    ErrorPackageNotInstalled, ErrorPackageNotFound,
    ErrorPackageAlreadyInstalled, ErrorPackageDownloadFailed,
    ErrorGroupNotFound, ErrorGroupListInvalid, ErrorDepResolutionFailed,
    ErrorFilterInvalid, ErrorCreateThreadFailed, ErrorTransactionError,
    ErrorTransactionCancelled, ErrorNoCache, ErrorRepoNotFound,
    ErrorCannotRemoveSystemPackage, ErrorProcessKill,
    ErrorFailedInitialization, ErrorFailedFinalise, ErrorFailedConfigParsing,
    ErrorCannotCancel, ErrorCannotGetLock, ErrorNoPackagesToUpdate,
    ErrorCannotWriteRepoConfig, ErrorLocalInstallFailed, ErrorBadGpgSignature,
    ErrorMissingGpgSignature, ErrorCannotInstallSourcePackage,
    ErrorRepoConfigurationError, ErrorNoLicenseAgreement, ErrorFileConflicts,
    ErrorPackageConflicts, ErrorRepoNotAvailable, ErrorInvalidPackageFile,
    ErrorPackageInstallBlocked, ErrorPackageCorrupt,
    ErrorAllPackagesAlreadyInstalled, ErrorFileNotFound,
    ErrorNoMoreMirrorsToTry, ErrorNoDistroUpgradeData,
    ErrorIncompatibleArchitecture, ErrorNoSpaceOnDevice,
    ErrorMediaChangeRequired, ErrorNotAuthorized, ErrorUpdateNotFound,
    ErrorCannotInstallRepoUnsigned, ErrorCannotUpdateRepoUnsigned,
    ErrorCannotGetFilelist, ErrorCannotGetRequires, ErrorCannotDisableRepository,
    ErrorRestrictedDownload, ErrorPackageFailedToConfigure,
    ErrorPackageFailedToBuild, ErrorPackageFailedToInstall,
    ErrorPackageFailedToRemove, ErrorUpdateFailedDueToRunningProcess,
    ErrorPackageDatabaseChanged, ErrorProvideTypeNotSupported,
    ErrorInstallRootInvalid, ErrorCannotFetchSources, ErrorCancelledPriority,
    ErrorUnfinishedTransaction, ErrorLockRequired, ErrorRepoAlreadySet
};

enum Group {
    GroupUnknown, GroupAccessibility, GroupAccessories, GroupAdminTools,
    GroupCommunication, GroupDesktopGnome, GroupDesktopKde, GroupDesktopOther,
    GroupDesktopXfce, GroupEducation, GroupFonts, GroupGames, GroupGraphics,
    GroupInternet, GroupLegacy, GroupLocalization, GroupMaps, GroupMultimedia,
    GroupNetwork, GroupOffice, GroupOther, GroupPowerManagement,
    GroupProgramming, GroupPublishing, GroupRepos, GroupSecurity, GroupServers,
    GroupSystem, GroupVirtualization, GroupScience, GroupDocumentation,
    GroupElectronics, GroupCollections, GroupVendor, GroupNewest,
    GroupDesktopDde, GroupLast
};

enum UpdateState {
    UpdateStateUnknown, UpdateStateStable, UpdateStateUnstable,
    UpdateStateTesting
};

enum Info {
    InfoUnknown, InfoInstalled, InfoAvailable, InfoLow, InfoEnhancement,
    InfoNormal, InfoBugfix, InfoImportant, InfoSecurity, InfoBlocked,
    InfoDownloading, InfoUpdating, InfoInstalling, InfoRemoving, InfoCleanup,
    InfoObsoleting, InfoCollectionInstalled, InfoCollectionAvailable,
    InfoFinished, InfoReinstalling, InfoDowngrading, InfoPreparing,
    InfoDecompressing, InfoUntrusted, InfoTrusted, InfoUnavailable,
    InfoCritical, InfoInstall, InfoRemove, InfoObsolete, InfoDowngrade
};

enum SigType { SigTypeUnknown, SigTypeGpg };

enum TransactionFlag {
    TransactionFlagNone, TransactionFlagOnlyTrusted, TransactionFlagSimulate,
    TransactionFlagOnlyDownload, TransactionFlagAllowReinstall,
    TransactionFlagJustReinstall, TransactionFlagAllowDowngrade
};

/* User-visible labels. */
TQString roleLabel(uint role);
TQString roleLabelPast(uint role);
TQString statusLabel(uint status);
TQString infoLabel(uint info);
TQString errorLabel(uint code);
TQString groupLabel(uint group);
TQString restartLabel(uint restart);

/* The name PackageKit uses for a group in SearchGroups, e.g. "admin-tools". */
TQString groupId(uint group);
/* The name of a group's icon in the TDE icon theme. */
TQString groupIcon(uint group);

/* Severity ordering for update lists: lower sorts first; security on top. */
int updateSeverityRank(uint info);

/* True for the Info values that describe an installed package. */
bool infoIsInstalled(uint info);

} // namespace Pk

#endif
