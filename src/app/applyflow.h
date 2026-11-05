/*
 * Applies queued changes or package files:
 *
 *   simulate each step -> confirm extras -> run each step
 *
 * Signature and license requests pause the current step, prompt, and retry it.
 * Every step is a separate PackageKit transaction, shown in one dialog.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef TDEPK_APPLYFLOW_H
#define TDEPK_APPLYFLOW_H

#include <tqobject.h>
#include <tqstringlist.h>
#include <tqvaluelist.h>

#include "pkenums.h"
#include "pktypes.h"

class ChangeQueue;
class PkTransaction;
class TQWidget;
class TransactionDialog;

class ApplyFlow : public TQObject
{
    TQ_OBJECT
public:
    explicit ApplyFlow(TQWidget *parent);

    void applyQueue(ChangeQueue *queue);
    void installFiles(const TQStringList &paths);

signals:
    /* The flow is over. `changed` means the system may have changed, so views should
       reload. Steps that succeeded have already been removed from the queue. */
    void done(bool changed);

private slots:
    void onPackage(const PkPackage &package);
    void onError(uint code, const TQString &details);
    void onSignature(const PkSignature &signature);
    void onEula(const PkEula &eula);
    void onRestart(uint type, const TQString &packageId);
    void onFinished(uint exit, uint runtime);
    void onPromptFinished(uint exit, uint runtime);
    /* These open dialogs, so they run outside the D-Bus callback. See pktransaction.h. */
    void continueAfterStep();
    void continueAfterPrompt();

private:
    enum Kind { RemoveStep, InstallStep, UpdateStep, FileStep };
    enum Phase { Simulating, Running, Prompting };
    struct Step {
        Kind kind;
        TQStringList ids;
        Step() : kind(InstallStep) {}
        Step(Kind k, const TQStringList &i) : kind(k), ids(i) {}
    };

    void start();
    void runCurrentStep();
    void afterSimulation();
    void confirmAndRun();
    void handleStepFailure(uint exit);
    void finish(bool changed);
    TQString stepTitle(const Step &step) const;
    bool isRequested(const PkPackage &package) const;

    TQWidget *m_parent;
    ChangeQueue *m_queue;
    TransactionDialog *m_dialog;
    PkTransaction *m_tx;
    TQValueList<Step> m_steps;
    uint m_index;
    Phase m_phase;
    PkPackageList m_simulated;
    TQStringList m_requestedNames;
    bool m_allowUntrusted;
    bool m_changed;
    uint m_retries;
    uint m_lastExit;

    uint m_errorCode;
    TQString m_errorDetails;
    bool m_hasSignature;
    PkSignature m_signature;
    bool m_hasEula;
    PkEula m_eula;
    uint m_restart;
    bool m_alwaysConfirm;
};

#endif
