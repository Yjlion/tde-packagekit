/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "confirmchangesdialog.h"

#include <tqlabel.h>
#include <tqlistview.h>
#include <tqvbox.h>

#include <kiconloader.h>
#include <kstdguiitem.h>
#include <tdelistview.h>
#include <tdelocale.h>

#include "packageicon.h"
#include "packageid.h"
#include "pkenums.h"

/* Groups for the list, most significant first. */
static TQString groupTitle(int index)
{
    switch (index) {
    case 0:  return i18n("To be removed");
    case 1:  return i18n("To be downgraded");
    case 2:  return i18n("To be installed");
    case 3:  return i18n("To be updated");
    case 4:  return i18n("To be reinstalled");
    default: return i18n("Other changes");
    }
}

static int groupIndex(uint info)
{
    switch (info) {
    case Pk::InfoRemoving:
    case Pk::InfoRemove:
    case Pk::InfoObsoleting:
    case Pk::InfoObsolete:    return 0;
    case Pk::InfoDowngrading:
    case Pk::InfoDowngrade:   return 1;
    case Pk::InfoInstalling:
    case Pk::InfoInstall:     return 2;
    case Pk::InfoUpdating:    return 3;
    case Pk::InfoReinstalling:return 4;
    default:                  return 5;
    }
}

ConfirmChangesDialog::ConfirmChangesDialog(const PkPackageList &changes, TQWidget *parent)
    : KDialogBase(parent, "confirmchanges", true, i18n("Confirm Changes"), Ok | Cancel, Ok, true)
{
    setButtonOK(KGuiItem(i18n("&Continue"), "ok"));

    TQVBox *page = makeVBoxMainWidget();
    TQLabel *intro = new TQLabel(i18n("These changes are needed to complete your request. "
                                      "Do you want to continue?"), page);
    intro->setAlignment(TQt::WordBreak);

    TDEListView *list = new TDEListView(page, "changes");
    list->addColumn(i18n("Package"));
    list->addColumn(i18n("Version"));
    list->addColumn(i18n("Summary"));
    list->setRootIsDecorated(true);
    list->setAllColumnsShowFocus(true);
    list->setSorting(-1);
    list->setColumnWidth(0, 220);
    list->setColumnWidth(1, 110);
    list->setColumnWidth(2, 260);

    TQListViewItem *last = nullptr;
    const int groups = 6;
    for (int g = 0; g < groups; ++g) {
        TQListViewItem *header = nullptr;
        uint count = 0;
        TQListViewItem *lastChild = nullptr;
        for (PkPackageList::const_iterator it = changes.begin(); it != changes.end(); ++it) {
            if (groupIndex((*it).info) != g) {
                continue;
            }
            if (!header) {
                header = last ? new TQListViewItem(list, last, groupTitle(g))
                              : new TQListViewItem(list, groupTitle(g));
                header->setOpen(true);
                header->setSelectable(false);
                header->setPixmap(0, SmallIcon(g == 0 ? "edit-delete" : "system-software-install"));
                last = header;
            }
            const PackageId id((*it).id);
            lastChild = new TQListViewItem(header, lastChild, id.name(), id.version(), (*it).summary);
            lastChild->setPixmap(0, PackageIcon::small(id.name()));
            ++count;
        }
        if (header) {
            header->setText(0, TQString("%1 (%2)").arg(groupTitle(g)).arg(count));
        }
    }
    resize(640, 400);
}

#include "confirmchangesdialog.moc"
