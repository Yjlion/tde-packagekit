/*
 * Prints the desktop entries TDE offers for a MIME type, one per line.
 * Used to check that the installed package really registered as a handler.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <stdio.h>

#include <kservice.h>
#include <kservicetype.h>
#include <tqstring.h>
#include <tdeaboutdata.h>
#include <tdeinstance.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <mimetype>\n", argv[0]);
        return 2;
    }
    TDEAboutData about("mime-offers", "mime-offers", "1");
    TDEInstance instance(&about);
    const KService::List offers = KServiceType::offers(argv[1]);
    for (KService::List::const_iterator it = offers.begin(); it != offers.end(); ++it) {
        printf("%s\n", (*it)->desktopEntryName().latin1());
    }
    return 0;
}
