 /*
    Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "AccountModel.h"

#include "tomahawksettings.h"
#include "accounts/AccountManager.h"
#include "accounts/Account.h"

#include "utils/logger.h"

using namespace Tomahawk;
using namespace Accounts;

AccountModel::AccountModel( QObject* parent )
    : QAbstractListModel( parent )
{
    connect( AccountManager::instance(), SIGNAL( added( Tomahawk::Accounts::Account* ) ), this, SLOT( accountAdded( Tomahawk::Accounts::Account* ) ) );
    connect( AccountManager::instance(), SIGNAL( removed( Tomahawk::Accounts::Account* ) ), this, SLOT( accountRemoved( Tomahawk::Accounts::Account* ) ) );
}


AccountModel::~AccountModel()
{
}


QVariant
AccountModel::data( const QModelIndex& index, int role ) const
{
    if( !index.isValid() )
        return QVariant();

    QList< Account* > accounts = AccountManager::instance()->accounts();
    Q_ASSERT( index.row() <= accounts.size() );
    Account* account = accounts[ index.row() ];
    switch( role )
    {
    case Qt::DisplayRole:
    case AccountModel::AccountName:
        return account->accountFriendlyName();
    case AccountModel::ConnectionStateRole:
        return account->connectionState();
    case AccountModel::HasConfig:
        return ( account->configurationWidget() != 0 );
    case AccountModel::AccountTypeRole:
        return (int)account->types();
    case Qt::DecorationRole:
        return account->icon();
    case AccountModel::AccountData:
        return QVariant::fromValue< QObject* >( account );
    case Qt::CheckStateRole:
        return account->enabled() ? Qt::Checked : Qt::Unchecked;
    default:
        return QVariant();
    }
    return QVariant();
}


bool
AccountModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    Q_ASSERT( index.isValid() && index.row() <= AccountManager::instance()->accounts().count() );

    if ( role == Qt::CheckStateRole ) {
        Qt::CheckState state = static_cast< Qt::CheckState >( value.toInt() );
        QList< Account* > accounts = AccountManager::instance()->accounts();
        Account* account = accounts[ index.row() ];

        if( state == Qt::Checked && !account->enabled() ) {
            account->setEnabled( true );
        } else if( state == Qt::Unchecked ) {
            account->setEnabled( false );
        }
        dataChanged( index, index );

        return true;
    }
    else if ( role == AccountTypeRole )
    {
        // TODO
    }
    return false;
}

int
AccountModel::rowCount( const QModelIndex& ) const
{
    return AccountManager::instance()->accounts().size();
}

Qt::ItemFlags
AccountModel::flags( const QModelIndex& index ) const
{
    return QAbstractListModel::flags( index ) | Qt::ItemIsUserCheckable;
}


void
AccountModel::accountAdded( Account* account )
{
    // TODO HACK we assume account plugins are added at the end of the list.
    Q_ASSERT( AccountManager::instance()->accounts().last() == account );
    if ( account->types() & SipType )
        connect( account, SIGNAL( connectionStateChanged( Tomahawk::Accounts::Account::ConnectionState ) ), this, SLOT( accountStateChanged( Tomahawk::Accounts::Account::ConnectionState ) ) );

    int size = AccountManager::instance()->accounts().count() - 1;
    beginInsertRows( QModelIndex(), size, size );
    endInsertRows();
}


void
AccountModel::accountRemoved( Account* account )
{
    int idx = AccountManager::instance()->accounts().indexOf( account );
    beginRemoveRows( QModelIndex(), idx, idx );
    endRemoveRows();
}


void
AccountModel::accountStateChanged( Tomahawk::Accounts::Account::ConnectionState )
{
    Account* account = qobject_cast< Account* >( sender() );
    Q_ASSERT( account );

    for ( int i = 0; i < AccountManager::instance()->accounts().size(); i++ )
    {
        if ( AccountManager::instance()->accounts()[i] == account )
        {
            QModelIndex idx = index( i, 0, QModelIndex() );
            emit dataChanged( idx, idx );
            return;
        }
    }
}

