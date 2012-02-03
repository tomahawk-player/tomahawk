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

   connect ( AccountManager::instance(), SIGNAL( added( Tomahawk::Accounts::Account* ) ), this, SLOT( accountAdded( Tomahawk::Accounts::Account* ) ) );
   connect ( AccountManager::instance(), SIGNAL( removed( Tomahawk::Accounts::Account* ) ), this, SLOT( accountRemoved( Tomahawk::Accounts::Account* ) ) );
   connect ( AccountManager::instance(), SIGNAL( stateChanged( Account* ,Accounts::Account::ConnectionState ) ), this, SLOT( accountStateChanged( Account*, Accounts::Account::ConnectionState ) ) );

}


QVariant
AccountModel::data( const QModelIndex& index, int role ) const
{
    if ( !index.isValid() )
        return QVariant();

    if ( !hasIndex( index.row(), index.column(), index.parent() ) )
        return QVariant();

    const AccountModelNode* node = nodeFromIndex( index );
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

                // If there's no account*, then it means it's a unique factory that hasn't been created
                if ( !acct )
                {
                    Q_ASSERT( node->type == AccountModelNode::UniqueFactoryType );
                    Q_ASSERT( node->factory );

                    switch( role )
                    {
                    case Qt::DisplayRole:
                        return node->factory->prettyName();
                    case Qt::DecorationRole:
                        return node->factory->icon();
                    case DescriptionRole:
                        return node->factory->description();
                    case RowTypeRole:
                        return TopLevelFactory;
                    case StateRole:
                        return Uninstalled;
                    default:
                        return QVariant();
                    }
                }
                else
                {
                    switch ( role )
                    {
                    case Qt::DisplayRole:
                        return acct->accountFriendlyName();
                    case Qt::DecorationRole:
                        return acct->icon();
                    case DescriptionRole:
                        return node->type == AccountModelNode::ManualResolverType ? QString() : node->factory->description();
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
                        return Installed;
                    default:
                        return QVariant();
                    }
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



bool
AccountModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if ( !index.isValid() || !hasIndex( index.row(), index.column(), index.parent() ) )
        return false;

    AccountModelNode* node = nodeFromIndex( index );

    if ( role == CheckboxClickedRole )
    {
        // All checkboxes are for turning on/off an account. So we can just do that
        Q_ASSERT( node->account || node->resolverAccount || node->atticaAccount );
        Q_ASSERT( node->type != AccountModelNode::FactoryType );

        Account* acct = 0;
        switch ( node->type )
        {
            case AccountModelNode::AccountType:
            case AccountModelNode::UniqueFactoryType:
                acct = node->account;
                break;
            case AccountModelNode::AtticaType:
                acct = node->atticaAccount;
                break;
            case AccountModelNode::ManualResolverType:
                acct = node->resolverAccount;
                break;
            default:
                ;
        };
        Q_ASSERT( acct );

        Qt::CheckState state = static_cast< Qt::CheckState >( value.toInt() );

        if ( state == Qt::Checked && !acct->enabled() )
            AccountManager::instance()->enableAccount( acct );
        else if( state == Qt::Unchecked )
            AccountManager::instance()->disableAccount( acct );

        acct->sync();
        emit dataChanged( index, index );

        return true;
    }

    // The install/create/remove/etc button was clicked. Handle it properly depending on this item
    if ( role == ButtonClickedRole )
    {
        switch ( node->type )
        {
            case AccountModelNode::FactoryType:
            case AccountModelNode::UniqueFactoryType:
            {
                Q_ASSERT( node->factory );

                // Make a new account of this factory type
                emit createAccount( node->factory );
                break;
            }
            case AccountModelNode::AccountType:
            case AccountModelNode::ManualResolverType:
            {
                Q_ASSERT( node->account || node->resolverAccount );
                Account* acct = node->type == AccountModelNode::AccountType ? node->account : node->resolverAccount;

                // This is a child account, and the remove button was just hit. Remove it!
                // OR this is a manually added resolver, and
                //  the only thing we can do with a manual resolver is remove it completely from the list
                AccountManager::instance()->removeAccount( acct );

                break;
            }
            case AccountModelNode::AtticaType:
            {
                // This is an attica resolver, may be installed or not. Handle it properly
                Q_ASSERT( node->atticaContent.isValid() );

                Attica::Content resolver = node->atticaContent;
                AtticaManager::ResolverState state = AtticaManager::instance()->resolverState( resolver );
                if ( role == Qt::EditRole )
                {
                    switch( state )
                    {
                        case AtticaManager::Uninstalled:
                            // install
                            AtticaManager::instance()->installResolver( resolver );
                            break;
                        case AtticaManager::Installing:
                        case AtticaManager::Upgrading:
                            // Do nothing, busy
                            break;
                        case AtticaManager::Installed:
                            // Uninstall
                            AtticaManager::instance()->uninstallResolver( resolver );
                            break;
                        case AtticaManager::NeedsUpgrade:
                            AtticaManager::instance()->upgradeResolver( resolver );
                            break;
                        default:
                            //FIXME -- this handles e.g. Failed
                            break;
                    };
                }
                emit dataChanged( index, index );
            }
        }

        return true;
    }
    if ( role == RatingRole )
    {
        // We only support rating Attica resolvers for the moment.
        Q_ASSERT( node->type == AccountModelNode::AtticaType );

        AtticaManager::ResolverState state = AtticaManager::instance()->resolverState( node->atticaContent );
        // For now only allow rating if a resolver is installed!
        if ( state != AtticaManager::Installed && state != AtticaManager::NeedsUpgrade )
            return false;
        if ( AtticaManager::instance()->userHasRated( node->atticaContent ) )
            return false;
        node->atticaContent.setRating( value.toInt() * 20 );
        AtticaManager::instance()->uploadRating( node->atticaContent );

        emit dataChanged( index, index );

        return true;
    }

    return false;
}


void
AccountModel::accountAdded( Account* account )
{
    // Find the factory this belongs up, and update
    AccountFactory* factory = AccountManager::instance()->factoryForAccount( account );
    for ( int i = 0; i < m_rootItem->children.size(); i++ )
    {
        AccountModelNode* n = m_rootItem->children.at( i );
        if ( n->factory == factory )
        {
            if ( factory->isUnique() )
            {
                Q_ASSERT( n->type == AccountModelNode::UniqueFactoryType );
                n->account = account;
                const QModelIndex idx = index( i, 0, QModelIndex() );
                emit dataChanged( idx, idx );

                return;
            }
            else
            {
                Q_ASSERT( n->type == AccountModelNode::FactoryType );
                // This is our parent
                beginInsertRows( index( i, 0, QModelIndex() ), n->children.size(), n->children.size() );
                new AccountModelNode( n, account );
                endInsertRows();

                return;
            }
        }
    }

    // Not matched with a factory. Then just add it at the end
    if ( AtticaResolverAccount* attica = qobject_cast< AtticaResolverAccount* >( account ) )
    {
        Attica::Content::List allAtticaContent = AtticaManager::instance()->resolvers();
        foreach ( const Attica::Content& c, allAtticaContent )
        {
            if ( attica->atticaId() == c.id() )
            {
                // This is us. Create the row
//                 const int count = m_rootItem->children.size()
//                 beginInsertRows( QModelIndex(), );
//                 new AccountModelNode(  );
            }
        }
    }
}


void
AccountModel::accountStateChanged( Account* account , Account::ConnectionState )
{
    // Find the factory this belongs up, and update
    AccountFactory* factory = AccountManager::instance()->factoryForAccount( account );
    for ( int i = 0; i < m_rootItem->children.size(); i++ )
    {
        AccountModelNode* n = m_rootItem->children.at( i );
        if ( n->type != AccountModelNode::FactoryType )
        {
            // If this is not a non-unique factory, it has as top-level account, so find that and update it
            // For each type that this node could be, check the corresponding data
            if ( ( n->type == AccountModelNode::UniqueFactoryType && n->account && n->account == account ) ||
                 ( n->type == AccountModelNode::AccountType && n->account == account ) ||
                 ( n->type == AccountModelNode::AtticaType && n->atticaAccount && n->atticaAccount == account ) ||
                 ( n->type == AccountModelNode::ManualResolverType && n->resolverAccount && n->resolverAccount == account ) )
            {
                const QModelIndex idx = index( i, 0, QModelIndex() );
                emit dataChanged( idx, idx );
            }
        }
        else
        {
            for ( int k = 0; k < n->children.size(); k++ )
            {
                AccountModelNode* childAccount = n->children.at( k );
                Q_ASSERT( childAccount->type == AccountModelNode::AccountType );
                if ( childAccount->account == account )
                {
                    const QModelIndex parent = index( i, 0, QModelIndex() );
                    const QModelIndex idx = index( k, 0, parent );
                    emit dataChanged( idx, idx );
                }
            }
        }

    }
}


void
AccountModel::accountRemoved( Account* account )
{
    // Find the factory this belongs up, and update
    AccountFactory* factory = AccountManager::instance()->factoryForAccount( account );
    for ( int i = 0; i < m_rootItem->children.size(); i++ )
    {
        AccountModelNode* n = m_rootItem->children.at( i );
        if ( n->factory == factory )
        {
            if ( factory->isUnique() )
            {
                Q_ASSERT( n->type == AccountModelNode::UniqueFactoryType );
                n->account = account;
                const QModelIndex idx = index( i, 0, QModelIndex() );
                emit dataChanged( idx, idx );
            }
            else
            {
                Q_ASSERT( n->type == AccountModelNode::FactoryType );
                // This is our parent
                beginInsertRows( index( i, 0, QModelIndex() ), n->children.size(), n->children.size() );
                new AccountModelNode( n, account );
                endInsertRows();
            }
        }
    }
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
