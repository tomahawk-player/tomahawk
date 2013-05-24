/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Teo Mrnjavac <teo@kde.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
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

#include "AccountModelFactoryProxy.h"

#include <QDebug>

using namespace Tomahawk;
using namespace Accounts;

AccountModelFactoryProxy::AccountModelFactoryProxy( QObject* parent )
    : QSortFilterProxyModel( parent )
    , m_filterEnabled( false )
    , m_filterRowType( AccountModel::TopLevelFactory )
{
    setDynamicSortFilter( true );
}


bool
AccountModelFactoryProxy::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
    if ( !m_filterEnabled )
        return true;

    const QModelIndex idx = sourceModel()->index( sourceRow, 0, sourceParent );

    const AccountModel::RowType rowType = static_cast< AccountModel::RowType >( idx.data( AccountModel::RowTypeRole ).value< int >() );

    if( rowType == Tomahawk::Accounts::AccountModel::TopLevelFactory )
    {
        if ( idx.data( Tomahawk::Accounts::AccountModel::ChildrenOfFactoryRole )
             .value< QList< Tomahawk::Accounts::Account* > >().isEmpty() )
            return false;

        Tomahawk::Accounts::AccountFactory* factory = qobject_cast< Tomahawk::Accounts::AccountFactory* >( idx.data( Tomahawk::Accounts::AccountModel::AccountData ).value< QObject* >() );
        if ( factory && factory->factoryId() == "twitteraccount" )
            return false;
    }

    return rowType == m_filterRowType;
}


void
AccountModelFactoryProxy::setFilterEnabled( bool enabled )
{
    m_filterEnabled = enabled;
    invalidate();
}


void
AccountModelFactoryProxy::setFilterRowType( AccountModel::RowType rowType )
{
    if( rowType == m_filterRowType )
        return;

    m_filterRowType = rowType;
    if( m_filterEnabled )
        invalidate();
}
