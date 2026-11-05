/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "applyflow.h"

#include <tqtimer.h>
#include <tqwidget.h>

#include <tdelocale.h>
#include <tdemessagebox.h>

#include "changequeue.h"
#include "confirmchangesdialog.h"
#include "euladialog.h"
#include "packageid.h"
#include "pktransaction.h"
#include "queryhelper.h"
#include "signaturedialog.h"
#include "transactiondialog.h"

/* A prompt can be followed by another one (several keys), but not forever. */
static const uint MaxRetries = 5;

ApplyFlow::ApplyFlow(TQWidget *parent)
    : TQObject(parent, "applyflow"),
      m_parent(parent), m_queue(nullptr), m_dialog(nullptr), m_tx(nullptr), m_index(0), m_phase(Simulating),
      m_allowUntrusted(false), m_changed(false), m_retries(0), m_lastExit(Pk::ExitUnknown),
      m_errorCode(0), m_hasSignature(false), m_hasEula(false), m_restart(Pk::RestartNone),
      m_alwaysConfirm(false)
{
}

void ApplyFlow::applyQueue(ChangeQueue *queue)
{
    m_queue = queue;
    m_steps.clear();
    m_requestedNames.clear();
    /* Removals first, so that freed disk space and resolved conflicts help the rest. */
    const TQStringList remove = queue->ids(ChangeQueue::Remove);
    const TQStringList install = queue->ids(ChangeQueue::Install);
    const TQStringList update = queue->ids(ChangeQueue::Update);
    if (!remove.isEmpty())  m_steps << Step(RemoveStep, remove);
    if (!install.isEmpty()) m_steps << Step(InstallStep, install);
    if (!update.isEmpty())  m_steps << Step(UpdateStep, update);

    TQStringList all = remove + install + update;
    for (TQStringList::const_iterator it = all.begin(); it != all.end(); ++it) {
        m_requestedNames << PackageId(*it).name();
    }
    /* Removing is always worth a second look. So is a mix of installs and updates,
       because updates ticked earlier on another page are easy to forget. */
    m_alwaysConfirm = !remove.isEmpty() || (!install.isEmpty() && !update.isEmpty());
    start();
}

void ApplyFlow::installFiles(const TQStringList &paths)
{
    m_queue = nullptr;
    m_steps.clear();
    m_requestedNames.clear();
    m_steps << Step(FileStep, paths);
    /* A file comes from outside the repositories, so always show what it does. */
    m_alwaysConfirm = true;
    start();
}

void ApplyFlow::start()
{
    if (m_steps.isEmpty()) {
        finish(false);
        return;
    }
    m_dialog = new TransactionDialog(m_parent);
    m_dialog->show();
    m_index = 0;
    m_phase = Simulating;
    m_simulated.clear();
    m_allowUntrusted = false;
    m_changed = false;
    m_retries = 0;
    m_restart = Pk::RestartNone;
    runCurrentStep();
}

TQString ApplyFlow::stepTitle(const Step &step) const
{
    const uint n = step.ids.count();
    if (m_phase == Simulating) {
        return i18n("Checking what will change...");
    }
    switch (step.kind) {
    case RemoveStep: return i18n("Removing 1 package", "Removing %n packages", n);
    case InstallStep: return i18n("Installing 1 package", "Installing %n packages", n);
    case UpdateStep: return i18n("Updating 1 package", "Updating %n packages", n);
    default: return i18n("Installing 1 file", "Installing %n files", n);
    }
}

void ApplyFlow::runCurrentStep()
{
    const Step step = m_steps[m_index];
    m_errorCode = 0;
    m_errorDetails = TQString();
    m_hasSignature = false;
    m_hasEula = false;

    m_tx = new PkTransaction(this);
    m_dialog->attach(m_tx, stepTitle(step));
    connect(m_tx, TQ_SIGNAL(errorCode(uint, const TQString &)), TQ_SLOT(onError(uint, const TQString &)));
    connect(m_tx, TQ_SIGNAL(signatureRequired(const PkSignature &)), TQ_SLOT(onSignature(const PkSignature &)));
    connect(m_tx, TQ_SIGNAL(eulaRequired(const PkEula &)), TQ_SLOT(onEula(const PkEula &)));
    connect(m_tx, TQ_SIGNAL(finished(uint, uint)), TQ_SLOT(onFinished(uint, uint)));

    Pk::Bitfield flags = 0;
    if (m_phase == Simulating) {
        flags |= Pk::bit(Pk::TransactionFlagSimulate);
        connect(m_tx, TQ_SIGNAL(package(const PkPackage &)), TQ_SLOT(onPackage(const PkPackage &)));
    } else {
        connect(m_tx, TQ_SIGNAL(restartRequired(uint, const TQString &)), TQ_SLOT(onRestart(uint, const TQString &)));
        if (!m_allowUntrusted) {
            flags |= Pk::bit(Pk::TransactionFlagOnlyTrusted);
        }
    }
    if (m_phase != Simulating) {
        m_changed = true;
    }

    switch (step.kind) {
    case RemoveStep:
        m_tx->removePackages(flags, step.ids, true, false);
        break;
    case InstallStep:
        m_tx->installPackages(flags, step.ids);
        break;
    case UpdateStep:
        m_tx->updatePackages(flags, step.ids);
        break;
    case FileStep:
        m_tx->installFiles(flags, step.ids);
        break;
    }
}

void ApplyFlow::onPackage(const PkPackage &package)
{
    switch (package.info) {
    case Pk::InfoFinished:
    case Pk::InfoCleanup:
    case Pk::InfoDownloading:
    case Pk::InfoPreparing:
    case Pk::InfoDecompressing:
        return;
    default:
        break;
    }
    /* The same package can be reported more than once. */
    for (PkPackageList::iterator it = m_simulated.begin(); it != m_simulated.end(); ++it) {
        if ((*it).id == package.id) {
            *it = package;
            return;
        }
    }
    m_simulated.append(package);
}

void ApplyFlow::onError(uint code, const TQString &details)
{
    m_errorCode = code;
    m_errorDetails = details;
}

void ApplyFlow::onSignature(const PkSignature &signature)
{
    m_signature = signature;
    m_hasSignature = true;
}

void ApplyFlow::onEula(const PkEula &eula)
{
    m_eula = eula;
    m_hasEula = true;
}

void ApplyFlow::onRestart(uint type, const TQString &)
{
    /* The values are ordered so that a larger one needs the bigger restart. */
    if (type > m_restart) {
        m_restart = type;
    }
}

void ApplyFlow::onFinished(uint exit, uint)
{
    m_dialog->detach();
    QueryHelper::release(m_tx);
    m_lastExit = exit;
    TQTimer::singleShot(0, this, TQ_SLOT(continueAfterStep()));
}

void ApplyFlow::continueAfterStep()
{
    const uint exit = m_lastExit;
    if (exit == Pk::ExitSuccess) {
        if (m_phase == Running && m_queue && m_steps[m_index].kind != FileStep) {
            m_queue->setMany(m_steps[m_index].ids, ChangeQueue::None);
        }
        if (m_index + 1 < m_steps.count()) {
            ++m_index;
            runCurrentStep();
        } else if (m_phase == Simulating) {
            afterSimulation();
        } else {
            finish(true);
        }
        return;
    }
    handleStepFailure(exit);
}

bool ApplyFlow::isRequested(const PkPackage &package) const
{
    return m_requestedNames.find(PackageId(package.id).name()) != m_requestedNames.end();
}

void ApplyFlow::afterSimulation()
{
    bool needsConfirmation = m_alwaysConfirm;
    for (PkPackageList::const_iterator it = m_simulated.begin(); it != m_simulated.end(); ++it) {
        if (!isRequested(*it)) {
            needsConfirmation = true;
        }
    }
    if (needsConfirmation && !m_simulated.isEmpty()) {
        m_dialog->hide();
        ConfirmChangesDialog confirm(m_simulated, m_parent);
        const bool accepted = confirm.exec() == TQDialog::Accepted;
        if (!accepted) {
            finish(false);
            return;
        }
        m_dialog->show();
    }
    m_phase = Running;
    m_index = 0;
    runCurrentStep();
}

void ApplyFlow::handleStepFailure(uint exit)
{
    if (exit == Pk::ExitCancelled || exit == Pk::ExitCancelledPriority) {
        finish(m_changed);
        return;
    }

    if ((m_hasSignature || m_hasEula) && m_retries < MaxRetries) {
        ++m_retries;
        m_dialog->hide();
        PkTransaction *prompt = nullptr;
        if (m_hasSignature) {
            SignatureDialog dialog(m_signature, m_parent);
            if (dialog.exec() == TQDialog::Accepted) {
                prompt = new PkTransaction(this);
                prompt->installSignature(m_signature.type, m_signature.keyId, m_signature.packageId);
            }
        } else {
            EulaDialog dialog(m_eula, m_parent);
            if (dialog.exec() == TQDialog::Accepted) {
                prompt = new PkTransaction(this);
                prompt->acceptEula(m_eula.id);
            }
        }
        if (!prompt) {
            finish(m_changed);
            return;
        }
        m_dialog->show();
        m_tx = prompt;
        m_errorCode = 0;
        m_errorDetails = TQString();
        m_dialog->attach(m_tx, i18n("Saving your choice..."));
        connect(m_tx, TQ_SIGNAL(errorCode(uint, const TQString &)), TQ_SLOT(onError(uint, const TQString &)));
        connect(m_tx, TQ_SIGNAL(finished(uint, uint)), TQ_SLOT(onPromptFinished(uint, uint)));
        return;
    }

    if (exit == Pk::ExitNeedUntrusted && !m_allowUntrusted) {
        m_dialog->hide();
        const int answer = KMessageBox::warningContinueCancel(
            m_parent,
            i18n("Some of this software could not be verified, so nothing guarantees where it "
                 "came from or that it was not altered. Install it only if you trust its source."),
            i18n("Unverified Software"), KGuiItem(i18n("&Install Anyway"), "messagebox_warning"));
        if (answer != KMessageBox::Continue) {
            finish(m_changed);
            return;
        }
        m_dialog->show();
        m_allowUntrusted = true;
        runCurrentStep();
        return;
    }

    m_dialog->hide();
    const TQString message = m_errorCode ? Pk::errorLabel(m_errorCode)
                                         : i18n("The package operation did not complete.");
    if (m_errorDetails.isEmpty()) {
        KMessageBox::error(m_parent, message, i18n("Could Not Apply Changes"));
    } else {
        KMessageBox::detailedError(m_parent, message, m_errorDetails, i18n("Could Not Apply Changes"));
    }
    finish(m_changed);
}

void ApplyFlow::onPromptFinished(uint exit, uint)
{
    m_dialog->detach();
    QueryHelper::release(m_tx);
    m_lastExit = exit;
    TQTimer::singleShot(0, this, TQ_SLOT(continueAfterPrompt()));
}

void ApplyFlow::continueAfterPrompt()
{
    if (m_lastExit != Pk::ExitSuccess) {
        handleStepFailure(m_lastExit);
        return;
    }
    runCurrentStep();
}

void ApplyFlow::finish(bool changed)
{
    if (m_dialog) {
        m_dialog->hide();
        m_dialog->deleteLater();
        m_dialog = nullptr;
    }
    const TQString restart = Pk::restartLabel(m_restart);
    if (!restart.isEmpty()) {
        KMessageBox::information(m_parent, restart, i18n("Restart Required"));
    }
    emit done(changed);
}

#include "applyflow.moc"
