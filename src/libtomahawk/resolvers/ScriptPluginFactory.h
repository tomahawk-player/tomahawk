/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright (C) 2015  Dominik Schmidt <domme@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef TOMAHAWK_SCRIPTPLUGINFACTORY_H
#define TOMAHAWK_SCRIPTPLUGINFACTORY_H

#include "../Typedefs.h"
#include "ScriptPlugin.h"
#include "ScriptAccount.h"

namespace Tomahawk
{

class ScriptAccount;
class ScriptCollection;

template <class T>
class ScriptPluginFactory
{
public:
    void registerPlugin( const scriptobject_ptr& object, ScriptAccount* scriptAccount )
    {
        if ( !m_scriptPlugins.value( object->id() ).isNull() )
            return;

        QSharedPointer< T > scriptPlugin = createPlugin( object, scriptAccount );
        if ( !scriptPlugin.isNull() )
        {
            m_scriptPlugins.insert( object->id(), scriptPlugin );

            if( !scriptAccount->isStopped() )
            {
                addPlugin( scriptPlugin );
            }
        }
    }


    void unregisterPlugin( const scriptobject_ptr& object )
    {
        QSharedPointer< T > scriptPlugin = m_scriptPlugins.value( object->id() );
        if ( !scriptPlugin.isNull() )
        {
            removePlugin( scriptPlugin );
        }

        m_scriptPlugins.remove( object->id() );
    }


    virtual QSharedPointer<T> createPlugin( const scriptobject_ptr&, ScriptAccount* )
    {
        return QSharedPointer<T>();
    }


    void addAllPlugins() const
    {
        foreach( const QWeakPointer< T >& scriptPlugin, m_scriptPlugins.values() )
        {
            addPlugin( scriptPlugin );
        }
    }

    virtual void addPlugin( const QSharedPointer< T >& scriptPlugin ) const
    {
    }


    void removeAllPlugins() const
    {
        foreach( const QWeakPointer< T >& scriptPlugin, m_scriptPlugins.values() )
        {
            removePlugin( scriptPlugin );
        }
    }


    virtual void removePlugin( const QSharedPointer<T>& scriptPlugin ) const
    {
    }


    const QHash< QString, QSharedPointer< T > > scriptPlugins() const
    {
        return m_scriptPlugins;
    }

private:
    QHash< QString, QSharedPointer< T > > m_scriptPlugins;
};

} // ns: Tomahawk

#endif // TOMAHAWK_SCRIPTPLUGINFACTORY_H
