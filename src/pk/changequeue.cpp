/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "changequeue.h"

ChangeQueue::ChangeQueue(TQObject *parent) : TQObject(parent, "changequeue") {}

void ChangeQueue::set(const TQString &packageId, Action action)
{
    TQMap<TQString, int>::iterator it = m_items.find(packageId);
    if (action == None) {
        if (it == m_items.end()) {
            return;
        }
        m_items.remove(it);
    } else {
        if (it != m_items.end() && it.data() == action) {
            return;
        }
        m_items.replace(packageId, action);
    }
    emit changed();
}

void ChangeQueue::setMany(const TQStringList &packageIds, Action action)
{
    bool any = false;
    for (TQStringList::const_iterator id = packageIds.begin(); id != packageIds.end(); ++id) {
        TQMap<TQString, int>::iterator it = m_items.find(*id);
        if (action == None) {
            if (it != m_items.end()) {
                m_items.remove(it);
                any = true;
            }
        } else if (it == m_items.end() || it.data() != action) {
            m_items.replace(*id, action);
            any = true;
        }
    }
    if (any) {
        emit changed();
    }
}

void ChangeQueue::clear()
{
    if (m_items.isEmpty()) {
        return;
    }
    m_items.clear();
    emit changed();
}

void ChangeQueue::clearAction(Action action)
{
    setMany(ids(action), None);
}

ChangeQueue::Action ChangeQueue::action(const TQString &packageId) const
{
    TQMap<TQString, int>::const_iterator it = m_items.find(packageId);
    return it == m_items.end() ? None : Action(it.data());
}

TQStringList ChangeQueue::ids(Action action) const
{
    TQStringList result;
    for (TQMap<TQString, int>::const_iterator it = m_items.begin(); it != m_items.end(); ++it) {
        if (it.data() == action) {
            result << it.key();
        }
    }
    return result;
}

uint ChangeQueue::count(Action action) const
{
    uint n = 0;
    for (TQMap<TQString, int>::const_iterator it = m_items.begin(); it != m_items.end(); ++it) {
        if (it.data() == action) {
            ++n;
        }
    }
    return n;
}

#include "changequeue.moc"
