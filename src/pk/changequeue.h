/*
 * The changes the user has asked for but not applied yet.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef TDEPK_CHANGEQUEUE_H
#define TDEPK_CHANGEQUEUE_H

#include <tqmap.h>
#include <tqobject.h>
#include <tqstring.h>
#include <tqstringlist.h>

class ChangeQueue : public TQObject
{
    TQ_OBJECT
public:
    enum Action { None, Install, Remove, Update };

    explicit ChangeQueue(TQObject *parent = nullptr);

    /* Action None removes the package from the queue. */
    void set(const TQString &packageId, Action action);
    /* Many changes as one changed() signal. */
    void setMany(const TQStringList &packageIds, Action action);
    void clear();
    /* Drops pending changes of one kind and leaves the rest. */
    void clearAction(Action action);

    Action action(const TQString &packageId) const;
    TQStringList ids(Action action) const;
    uint count() const { return m_items.count(); }
    uint count(Action action) const;
    bool isEmpty() const { return m_items.isEmpty(); }

signals:
    void changed();

private:
    TQMap<TQString, int> m_items;
};

#endif
