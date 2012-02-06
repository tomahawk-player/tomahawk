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
    : QAbstractListModel( parent )
{
    loadData();
}

void
AccountModel::loadData()
{
    beginResetModel();

    qDeleteAll( m_accounts );

    // Add all factories
    QList< AccountFactory* > factories = AccountManager::instance()->factories();
    QList< Account* > allAccounts = AccountManager::instance()->accounts();
    foreach ( AccountFactory* fac, factories )
    {
        if ( !fac->allowUserCreation() )
            continue;

        qDebug() << "Creating factory node:" << fac->prettyName();
        m_accounts << new AccountModelNode( fac );
    }

    // add all attica resolvers (installed or uninstalled)
    Attica::Content::List fromAttica = AtticaManager::instance()->resolvers();
    foreach ( const Attica::Content& content, fromAttica )
        m_accounts << new AccountModelNode( content );

    // Add all non-attica manually installed resolvers
   foreach ( Account* acct, allAccounts )
   {
       if ( qobject_cast< ResolverAccount* >( acct ) && !qobject_cast< AtticaResolverAccount* >( acct ) )
       {
           m_accounts << new AccountModelNode( qobject_cast< ResolverAccount* >( acct ) );
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

    const AccountModelNode* node = m_accounts.at( index.row() );
    // This is a top-level item. 3 cases

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
                case CanRateRole:
                    return false;
                case RowTypeRole:
                    return TopLevelFactory;
                case AccountData:
                    return QVariant::fromValue< QObject* >( node->factory );
                case ChildrenOfFactoryRole:
                    return QVariant::fromValue< QList< Tomahawk::Accounts::Account* > >( node->accounts );
                case HasConfig:
                    return !node->accounts.isEmpty();
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
                case CanRateRole:
                    return true;
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
            if ( role == RowTypeRole )
                return UniqueFactory;

            Account* acct = 0;
            if ( node->type == AccountModelNode::ManualResolverType )
                acct = node->resolverAccount;
            else if ( node->type == AccountModelNode::UniqueFactoryType )
                acct = node->accounts.first();

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
                case StateRole:
                    return Uninstalled;
                case CanRateRole:
                    return false;
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
                case ConnectionStateRole:
                    return acct->connectionState();
                case HasConfig:
                    return acct->configurationWidget() != 0;
                case StateRole:
                    return Installed;
                case ChildrenOfFactoryRole:
                    return QVariant::fromValue< QList< Tomahawk::Accounts::Account* > >( node->accounts );
                default:
                    return QVariant();
                }
            }
        }
    }

    return QVariant();
}



bool
AccountModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if ( !index.isValid() || !hasIndex( index.row(), index.column(), index.parent() ) )
        return false;

    AccountModelNode* node = m_accounts.at( index.row() );

    if ( role == CheckboxClickedRole )
    {
        Account* acct = 0;
        switch ( node->type )
        {
            case AccountModelNode::UniqueFactoryType:
                Q_ASSERT( node->accounts.size() == 1 );
                acct = node->accounts.first();
                break;
            case AccountModelNode::AtticaType:
            {
                // This may or may not be installed. if it's not installed yet, install it, then go ahead and enable it
                Q_ASSERT( node->atticaContent.isValid() );

                Attica::Content resolver = node->atticaContent;
                AtticaManager::ResolverState state = AtticaManager::instance()->resolverState( resolver );
                if ( state == AtticaManager::Installed )
                {
                    acct = node->atticaAccount;
                    break;
                }
                else
                {
                    connect( AtticaManager::instance(), SIGNAL( resolverInstalled( QString ) ), this, SLOT( atticaInstalled( QString ) ) );
                    m_waitingForAtticaInstall.insert( resolver.id() );

                    AtticaManager::instance()->installResolver( resolver );
                    return true;
                }

            }
            case AccountModelNode::ManualResolverType:
                acct = node->resolverAccount;
                break;
            default:
                ;
        };

        if ( node->type == AccountModelNode::FactoryType )
        {
            // TODO handle overall on/off

            return false;
        }

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
    if ( role == AddAccountButtonRole )
    {
        Q_ASSERT( node->type == AccountModelNode::FactoryType );
        // Make a new account of this factory type
        emit createAccount( node->factory );
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
    for ( int i = 0; i < m_accounts.size(); i++ )
    {
        AccountModelNode* n = m_accounts.at( i );
        if ( n->factory == factory )
        {
            n->accounts << account;
            const QModelIndex idx = index( i, 0, QModelIndex() );
            dataChanged( idx, idx );

            return;
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
                const int count = m_accounts.size();
                beginInsertRows( QModelIndex(), count, count );
                m_accounts << new AccountModelNode( c );
                endInsertRows();

                return;
            }
        }
    }

    // Ok, just a plain resolver. add it at the end
    if ( ResolverAccount* resolver = qobject_cast< ResolverAccount* >( account ) )
    {
        const int count = m_accounts.size();
        beginInsertRows( QModelIndex(), count, count );
        m_accounts << new AccountModelNode( resolver );
        endInsertRows();
    }
}


void
AccountModel::accountStateChanged( Account* account , Account::ConnectionState )
{
    // Find the factory this belongs up, and update
    AccountFactory* factory = AccountManager::instance()->factoryForAccount( account );
    for ( int i = 0; i < m_accounts.size(); i++ )
    {
        AccountModelNode* n = m_accounts.at( i );
        if ( n->type != AccountModelNode::FactoryType )
        {
            // If this is not a non-unique factory, it has as top-level account, so find that and update it
            // For each type that this node could be, check the corresponding data
            if ( ( n->type == AccountModelNode::UniqueFactoryType && n->accounts.size() && n->accounts.first() == account ) ||
                 ( n->type == AccountModelNode::AtticaType && n->atticaAccount && n->atticaAccount == account ) ||
                 ( n->type == AccountModelNode::ManualResolverType && n->resolverAccount && n->resolverAccount == account ) )
            {
                const QModelIndex idx = index( i, 0, QModelIndex() );
                emit dataChanged( idx, idx );
            }
        }
        else
        {
            for ( int k = 0; k < n->accounts.size(); k++ )
            {
                Account* childAccount = n->accounts.at( k );

                if ( childAccount == account )
                {
                    const QModelIndex idx = index( i, 0, QModelIndex() );
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
    for ( int i = 0; i < m_accounts.size(); i++ )
    {
        AccountModelNode* n = m_accounts.at( i );

        if ( n->type == AccountModelNode::FactoryType &&
             n->factory == factory )
        {
            n->accounts.removeAll( account );
            const QModelIndex idx = index( i, 0, QModelIndex() );
            emit dataChanged( idx, idx );

            return;
        }

        if ( ( n->type == AccountModelNode::UniqueFactoryType && n->accounts.size() && n->accounts.first() == account ) ||
             ( n->type == AccountModelNode::AtticaType && n->atticaAccount && n->atticaAccount == account ) ||
             ( n->type == AccountModelNode::ManualResolverType && n->resolverAccount && n->resolverAccount == account ) )
        {
            beginRemoveRows( QModelIndex(), i, i );
            m_accounts.removeAt( i );
            endRemoveRows();

            return;
        }
    }
}


void
AccountModel::atticaInstalled( const QString& atticaId )
{

}


int
AccountModel::rowCount( const QModelIndex& ) const
{
    return m_accounts.size();
}

