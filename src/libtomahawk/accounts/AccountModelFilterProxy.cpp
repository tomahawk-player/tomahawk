/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "AccountModelFilterProxy.h"

#include "AccountModel.h"


using namespace Tomahawk;
using namespace Accounts;

AccountModelFilterProxy::AccountModelFilterProxy( QObject* parent )
    : QSortFilterProxyModel(parent)
    , m_filterType( NoType )
{

}


void
AccountModelFilterProxy::setSourceModel( QAbstractItemModel* sourceModel )
{
    connect( sourceModel, SIGNAL( scrollTo( QModelIndex ) ), this, SLOT( onScrollTo( QModelIndex ) ) );
    connect( sourceModel, SIGNAL( startInstalling( QPersistentModelIndex ) ), this, SLOT( onStartInstalling( QPersistentModelIndex ) ) );
    connect( sourceModel, SIGNAL( doneInstalling( QPersistentModelIndex ) ), this, SLOT( onDoneInstalling( QPersistentModelIndex ) ) );
    connect( sourceModel, SIGNAL( errorInstalling( QPersistentModelIndex ) ), this, SLOT( onErrorInstalling( QPersistentModelIndex ) ) );
    QSortFilterProxyModel::setSourceModel( sourceModel );
}


bool
AccountModelFilterProxy::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
    if ( m_filterType == NoType )
        return true;

    const QModelIndex idx = sourceModel()->index( sourceRow, 0, sourceParent );
    const AccountTypes types = static_cast< AccountTypes >( idx.data( AccountModel::AccountTypeRole ).value< AccountTypes >() );

    return types.testFlag( m_filterType );

}


void
AccountModelFilterProxy::setFilterType( AccountType type )
{
    if ( type == m_filterType )
        return;

    m_filterType = type;
    invalidate();
}


void
AccountModelFilterProxy::onScrollTo( const QModelIndex& idx )
{
    emit scrollTo( mapFromSource( idx ) );
}


void
AccountModelFilterProxy::onDoneInstalling( const QPersistentModelIndex& idx )
{
    emit doneInstalling( mapFromSource( idx ) );
}


void
AccountModelFilterProxy::onStartInstalling( const QPersistentModelIndex& idx )
{
    emit startInstalling( mapFromSource( idx ) );
}


void
AccountModelFilterProxy::onErrorInstalling( const QPersistentModelIndex& idx )
{
    emit errorInstalling( mapFromSource( idx ) );
}

