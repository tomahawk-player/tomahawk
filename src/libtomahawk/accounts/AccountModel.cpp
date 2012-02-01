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

#include "AccountModel.h"

#include "Account.h"
#include "AccountModelNode.h"
#include "AccountManager.h"
#include "AtticaManager.h"
#include "ResolverAccount.h"

#include <attica/content.h>

using namespace Tomahawk;
using namespace Accounts;

AccountModel::AccountModel( QObject* parent )
    : QAbstractItemModel( parent )
    , m_rootItem( 0 )
{
    loadData();
}

void
AccountModel::loadData()
{
    beginResetModel();

    delete m_rootItem;

    m_rootItem = new AccountModelNode();
    // Add all factories
    QList< AccountFactory* > factories = AccountManager::instance()->factories();
    QList< Account* > allAccounts = AccountManager::instance()->accounts();
    foreach ( AccountFactory* fac, factories )
    {
        if ( !fac->allowUserCreation() )
            continue;

        qDebug() << "Creating factory node:" << fac->prettyName();
        new AccountModelNode( m_rootItem, fac );
    }

    // add all attica resolvers (installed or uninstalled)
    Attica::Content::List fromAttica = AtticaManager::instance()->resolvers();
    foreach ( const Attica::Content& content, fromAttica )
        new AccountModelNode( m_rootItem, content );

    // Add all non-attica manually installed resolvers
   foreach ( Account* acct, allAccounts )
   {
       if ( qobject_cast< ResolverAccount* >( acct ) && !qobject_cast< AtticaResolverAccount* >( acct ) )
       {
           new AccountModelNode( m_rootItem, qobject_cast< ResolverAccount* >( acct ) );
       }
   }

}


QVariant
AccountModel::data( const QModelIndex& index, int role ) const
{
    if ( !index.isValid() )
        return QVariant();

    if ( !hasIndex( index.row(), index.column(), index.parent() ) )
        return QVariant();

    AccountModelNode* node = nodeFromIndex( index );
    if ( node->parent == m_rootItem ) {
        // This is a top-level item. 3 cases
        Q_ASSERT( node->type != AccountModelNode::AccountType ); // must not be of this type, these should be children (other branch of if)

        switch ( node->type )
        {
            case AccountModelNode::FactoryType:
            {
                AccountFactory* fac = node->factory;
                Q_ASSERT( fac );

                switch ( role )
                {
                    case Qt::DisplayRole:
                        return fac->prettyName();
                    case Qt::DecorationRole:
                        return fac->icon();
                    case StateRole:
                        return ShippedWithTomahawk;
                    case DescriptionRole:
                        return fac->description();
                    case AuthorRole:
                        return "Tomahawk Team";
                    case RowTypeRole:
                        return TopLevelFactory;
                    default:
                        return QVariant();
                }
            }
            case AccountModelNode::AtticaType:
            {
                Attica::Content c = node->atticaContent;
                Q_ASSERT( !c.id().isNull() );

                switch( role )
                {
                    case Qt::DisplayRole:
                        return c.name();
                    case Qt::DecorationRole:
                        return QVariant::fromValue< QPixmap >( AtticaManager::instance()->iconForResolver( c ) );
                    case StateRole:
                        return (int)AtticaManager::instance()->resolverState( c );
                    case DescriptionRole:
                        return c.description();
                    case AuthorRole:
                        return c.author();
                    case RowTypeRole:
                        return TopLevelAccount;
                    case RatingRole:
                        return c.rating() / 20; // rating is out of 100
                    case DownloadCounterRole:
                        return c.downloads();
                    case VersionRole:
                        return c.version();
                    case UserHasRatedRole:
                        return AtticaManager::instance()->userHasRated( c );
                    default:
                        ;
                }

                AtticaResolverAccount* atticaAcct = node->atticaAccount;
                if ( atticaAcct )
                {
                    // If the resolver is installed or on disk, we expose some additional data
                    switch ( role )
                    {
                        case HasConfig:
                            return atticaAcct->configurationWidget() != 0;
                        case Qt::CheckStateRole:
                            return atticaAcct->enabled() ? Qt::Checked : Qt::Unchecked;
                        case AccountData:
                            return QVariant::fromValue< QObject* >( atticaAcct );
                        case ConnectionStateRole:
                            return atticaAcct->connectionState();
                        default:
                            ;
                    }
                }
                return QVariant();
            }
            case AccountModelNode::ManualResolverType:
            case AccountModelNode::UniqueFactoryType:
            {
                Account* acct = 0;
                if ( node->type == AccountModelNode::ManualResolverType )
                    acct = node->resolverAccount;
                else if ( node->type == AccountModelNode::UniqueFactoryType )
                    acct = node->account;

                Q_ASSERT( acct );

                switch ( role )
                {
                    case Qt::DisplayRole:
                        return acct->accountFriendlyName();
                    case Qt::DecorationRole:
                        return acct->icon();
                    case DescriptionRole:
                        return QString();
                    case Qt::CheckStateRole:
                        return acct->enabled() ? Qt::Checked : Qt::Unchecked;
                    case AccountData:
                        return QVariant::fromValue< QObject* >( acct );
                    case RowTypeRole:
                        return TopLevelAccount;
                    case ConnectionStateRole:
                        return acct->connectionState();
                    case HasConfig:
                        return acct->configurationWidget() != 0;
                    case StateRole:
                        return node->type == AccountModelNode::ManualResolverType ? Installed : UniqueFactory;
                    default:
                        return QVariant();
                }
            }
            case AccountModelNode::AccountType:
                Q_ASSERT( false ); // Should not be here---all account nodes should be children of top level nodes
        }
    }
    else
    {
        // This is a child account* of an accountfactory*
        Q_ASSERT( node->type == AccountModelNode::AccountType );
        Q_ASSERT( node->children.isEmpty() );
        Q_ASSERT( node->account );

        Account* acc = node->account;
        switch ( role )
        {
            case RowTypeRole:
                return ChildAccount;
            case Qt::DisplayRole:
                return acc->accountFriendlyName();
            case ConnectionStateRole:
                return acc->connectionState();
            case HasConfig:
                return ( acc->configurationWidget() != 0 );
            case ErrorString:
                return acc->errorMessage();
            case Qt::CheckStateRole:
                return acc->enabled() ? Qt::Checked : Qt::Unchecked;
            case AccountData:
                return QVariant::fromValue< QObject* >( acc );
            default:
                return QVariant();
        }
    }

    return QVariant();
}

int
AccountModel::columnCount( const QModelIndex& parent ) const
{
    return 1;
}

int
AccountModel::rowCount( const QModelIndex& parent ) const
{
    if ( !parent.isValid() )
    {
        return m_rootItem->children.count();
    }

    // If it's a top-level item, return child count. Only factories will have any.
    return nodeFromIndex( parent )->children.count();
}

QModelIndex
AccountModel::parent( const QModelIndex& child ) const
{
    if ( !child.isValid() )
    {
        return QModelIndex();
    }

    AccountModelNode* node = nodeFromIndex( child );
    AccountModelNode* parent = node->parent;

    // top level, none
    if( parent == m_rootItem )
        return QModelIndex();

    // child Account* of an AccountFactory*
    Q_ASSERT( m_rootItem->children.contains( parent ) );
    return createIndex( m_rootItem->children.indexOf( parent ), 0, parent );
}

QModelIndex
AccountModel::index( int row, int column, const QModelIndex& parent ) const
{
    if( row < 0 || column < 0 )
        return QModelIndex();

    if( hasIndex( row, column, parent ) )
    {
        AccountModelNode *parentNode = nodeFromIndex( parent );
        AccountModelNode *childNode = parentNode->children.at( row );
        return createIndex( row, column, childNode );
    }

    return QModelIndex();
}

AccountModelNode*
AccountModel::nodeFromIndex( const QModelIndex& idx ) const
{
    if( !idx.isValid() )
        return m_rootItem;

    Q_ASSERT( idx.internalPointer() );

    return reinterpret_cast< AccountModelNode* >( idx.internalPointer() );
}
