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
    connect( AtticaManager::instance(), SIGNAL( resolversLoaded( Attica::Content::List ) ), this, SLOT( loadData() ) );

    connect( AccountManager::instance(), SIGNAL( added( Tomahawk::Accounts::Account* ) ), this, SLOT( accountAdded( Tomahawk::Accounts::Account* ) ) );
    connect( AccountManager::instance(), SIGNAL( removed( Tomahawk::Accounts::Account* ) ), this, SLOT( accountRemoved( Tomahawk::Accounts::Account* ) ) );
    connect( AccountManager::instance(), SIGNAL( stateChanged( Account* ,Accounts::Account::ConnectionState ) ), this, SLOT( accountStateChanged( Account*, Accounts::Account::ConnectionState ) ) );

    loadData();
}

void
AccountModel::loadData()
{
    beginResetModel();

    qDeleteAll( m_accounts );
    m_accounts.clear();

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
    {
        qDebug() << "Loading ATTICA ACCOUNT with content:" << content.id() << content.name();
        m_accounts << new AccountModelNode( content );
    }

    // Add all non-attica manually installed resolvers
   foreach ( Account* acct, allAccounts )
   {
       if ( qobject_cast< ResolverAccount* >( acct ) && !qobject_cast< AtticaResolverAccount* >( acct ) )
       {
           m_accounts << new AccountModelNode( qobject_cast< ResolverAccount* >( acct ) );
       }
   }

   endResetModel();
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
                case Qt::ToolTipRole:
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
                case AccountTypeRole:
                    return QVariant::fromValue< AccountTypes >( node->factory->types() );
                case Qt::CheckStateRole:
                {
                    if ( node->accounts.isEmpty() )
                        return Qt::Unchecked;

                    // If all are checked or unchecked, return that
                    bool someOn = false, someOff = false;
                    foreach ( const Account* acct, node->accounts )
                    {
                        if ( acct->enabled() )
                            someOn = true;
                        else
                            someOff = true;
                    }
                    if ( someOn && !someOff )
                        return Qt::Checked;
                    else if ( someOff & !someOn )
                        return Qt::Unchecked;
                    else
                        return Qt::PartiallyChecked;
                }
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
                case Qt::ToolTipRole:
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
                case AccountTypeRole:
                    return QVariant::fromValue< AccountTypes >( AccountTypes( ResolverType ) );
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
            {
                if ( node->type == AccountModelNode::ManualResolverType )
                    return TopLevelAccount;
                else
                    return UniqueFactory;
            }
            else if ( role == CanDeleteRole )
            {
                return node->type == AccountModelNode::ManualResolverType;
            }

            Account* acct = 0;
            if ( node->type == AccountModelNode::ManualResolverType )
                acct = node->resolverAccount;
            else if ( node->type == AccountModelNode::UniqueFactoryType )
                acct = node->accounts.isEmpty() ? 0 : node->accounts.first();

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
                case Qt::ToolTipRole:
                case DescriptionRole:
                    return node->factory->description();
                case StateRole:
                    return Uninstalled;
                case CanRateRole:
                    return false;
                case AccountTypeRole:
                    return QVariant::fromValue< AccountTypes >( node->factory->types() );
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
                case AccountTypeRole:
                    return QVariant::fromValue< AccountTypes >( acct->types() );
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
                if ( node->accounts.isEmpty() )
                {
                    // No account for this unique factory, create it
                    // Don't add it to node->accounts here, slot attached to accountmanager::accountcreated will do it for us
                    acct = node->factory->createAccount();
                    AccountManager::instance()->addAccount( acct );
                    TomahawkSettings::instance()->addAccount( acct->accountId() );
                }
                break;
            case AccountModelNode::AtticaType:
            {
                // This may or may not be installed. if it's not installed yet, install it, then go ahead and enable it
                Q_ASSERT( node->atticaContent.isValid() );

                Attica::Content resolver = node->atticaContent;
                AtticaManager::ResolverState state = AtticaManager::instance()->resolverState( resolver );
                qDebug() << "Attica resolver was checked! Current state is:" << state << "and so..";
                if ( state == AtticaManager::Installed && !node->resolverAccount )
                {
                    // Something is wrong, reinstall
                    qDebug() << "Found installed state but no resolver, uninstalling first";
                    AtticaManager::instance()->uninstallResolver( resolver );
                    state = AtticaManager::Uninstalled;
                }

                if ( state == AtticaManager::Installed )
                {
                    qDebug() << "Already installed with resolver, just enabling";
                    acct = node->atticaAccount;
                    break;
                }
                else
                {
                    qDebug() << "Kicked off fetch+install, now waiting";
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
            // Turn on or off all accounts for this factory

            Qt::CheckState state = static_cast< Qt::CheckState >( value.toInt() );

            foreach ( Account* acct, node->accounts )
            {
                state == Qt::Checked ? AccountManager::instance()->enableAccount( acct )
                                     : AccountManager::instance()->disableAccount( acct );
            }

            emit dataChanged( index, index );
            return true;
        }

        Q_ASSERT( acct );
        Qt::CheckState state = static_cast< Qt::CheckState >( value.toInt() );

        if ( state == Qt::Checked && !acct->enabled() )
            AccountManager::instance()->enableAccount( acct );
        else if( state == Qt::Unchecked )
            AccountManager::instance()->disableAccount( acct );

        emit dataChanged( index, index );

        return true;
    }

    // The install/create/remove/etc button was clicked. Handle it properly depending on this item
    if ( role == CustomButtonRole )
    {
        if ( node->type == AccountModelNode::FactoryType )
        {
            // Make a new account of this factory type
            emit createAccount( node->factory );
            return true;
        }
        else if ( node->type == AccountModelNode::ManualResolverType )
        {
            Q_ASSERT( node->resolverAccount );
            AccountManager::instance()->removeAccount( node->resolverAccount );

            return true;
        }
        Q_ASSERT( false ); // Should not be here, only the above two types should have this button
        return false;
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
    qDebug() << "IN ACCOUNT ADDED, new account:" << account->accountFriendlyName();
    AccountFactory* factory = AccountManager::instance()->factoryForAccount( account );
    AtticaResolverAccount* attica = qobject_cast< AtticaResolverAccount* >( account );
    for ( int i = 0; i < m_accounts.size(); i++ )
    {
        AccountModelNode* n = m_accounts.at( i );
        bool thisIsTheOne = false;
        qDebug() << "Checking for added account's related factory or attica:" << n->factory << attica;
        if ( attica )
            qDebug() << n->atticaContent.id() << n->atticaContent.name() << attica->atticaId();
        if ( n->factory == factory )
        {
            n->accounts << account;
            thisIsTheOne = true;
        }
        else if ( attica && n->atticaContent.id() == attica->atticaId() )
        {

            n->resolverAccount = attica;
            n->atticaContent = AtticaManager::instance()->resolverForId( attica->atticaId() );
            thisIsTheOne = true;

            if ( m_waitingForAtticaInstall.contains( attica->atticaId() ) )
                AccountManager::instance()->enableAccount( account );

            m_waitingForAtticaInstall.remove( attica->atticaId() );
        }

        if ( thisIsTheOne )
        {
            const QModelIndex idx = index( i, 0, QModelIndex() );
            dataChanged( idx, idx );

            return;
        }
    }

    // Ok, just a plain resolver. add it at the end
    if ( ResolverAccount* resolver = qobject_cast< ResolverAccount* >( account ) )
    {
        Q_ASSERT( qobject_cast< AtticaResolverAccount* >( account ) == 0 ); // should NOT get attica accounts here, should be caught above
        const int count = m_accounts.size();
        beginInsertRows( QModelIndex(), count, count );
        m_accounts << new AccountModelNode( resolver );
        endInsertRows();

        emit scrollTo( index( m_accounts.size() - 1, 0, QModelIndex() ) );
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
    // Find the row this belongs to and update/remove
    AccountFactory* factory = AccountManager::instance()->factoryForAccount( account );
    qDebug() << "AccountModel got account removed:" << account->accountFriendlyName();
    for ( int i = 0; i < m_accounts.size(); i++ )
    {
        AccountModelNode* n = m_accounts.at( i );

        bool found = false;
        // Account in a factory, remove child and update
        if ( ( n->type == AccountModelNode::FactoryType && n->factory == factory ) ||
             ( n->type == AccountModelNode::UniqueFactoryType && n->accounts.size() && n->accounts.first() == account ) )
        {
            n->accounts.removeAll( account );
            found = true;
        }

        // Attica account, just clear the account but leave the attica shell
        if ( n->type == AccountModelNode::AtticaType && n->atticaAccount && n->atticaAccount == account )
        {
            n->resolverAccount = 0;
            found = true;
        }

        if ( found )
        {
            qDebug() << "Found account removed but we don't want to delete a row!" << i << n->type << n->factory;
            const QModelIndex idx = index( i, 0, QModelIndex() );
            emit dataChanged( idx, idx );

            return;
        }

        // Manual resolver added, remove the row now
        if ( n->type == AccountModelNode::ManualResolverType && n->resolverAccount && n->resolverAccount == account )
        {
            qDebug() << "Found account removed AND REMOVING IT FROM THE LIST!" << n->factory << n->type << n->accounts << i;

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
    if ( !m_waitingForAtticaInstall.contains( atticaId ) )
        return;

    m_waitingForAtticaInstall.remove( atticaId );

    // find associated Account*, set on to the saved resolver, and update state
    AccountModelNode* node = 0;
    AtticaResolverAccount* acct = 0;

    foreach ( AccountModelNode* n, m_accounts )
    {
        if ( n->type == AccountModelNode::AtticaType &&
             n->atticaContent.id() == atticaId )
        {
            node = n;
            break;
        }
    }

    if ( !node )
    {
        Q_ASSERT( false );
        return; // Couldn't find it??
    }

    foreach ( Account* acc, AccountManager::instance()->accounts( ResolverType ) )
    {
        if ( AtticaResolverAccount* ra = qobject_cast< AtticaResolverAccount* >( acc ) )
        {
            if ( ra->atticaId() == atticaId )
            {
                acct = ra;
                break;
            }
        }
    }

    if ( !acct )
    {
        qWarning() << "Got installed attica resolver but couldnt' find a resolver account for it??";
        return;
    }

    AccountManager::instance()->enableAccount( acct );

    node->atticaAccount = acct;
    node->atticaContent = AtticaManager::instance()->resolverForId( atticaId );
    const QModelIndex idx = index( m_accounts.indexOf( node ), 0, QModelIndex() );
    emit dataChanged( idx, idx );
}


int
AccountModel::rowCount( const QModelIndex& ) const
{
    return m_accounts.size();
}

