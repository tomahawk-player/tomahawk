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
#include "TomahawkSettings.h"
#include "utils/Logger.h"

#ifndef ENABLE_HEADLESS
#include <QMessageBox>
#endif

#include <attica/content.h>

using namespace Tomahawk;
using namespace Accounts;

#define ACCOUNTMODEL_DEBUG 0

AccountModel::AccountModel( QObject* parent )
    : QAbstractListModel( parent )
    , m_waitingForAtticaLoaded( true )
{
    connect( AtticaManager::instance(), SIGNAL( resolversLoaded( Attica::Content::List ) ), this, SLOT( atticaLoaded() ) );
    connect( AtticaManager::instance(), SIGNAL( startedInstalling( QString ) ), this, SLOT( onStartedInstalling( QString ) ) );
    connect( AtticaManager::instance(), SIGNAL( resolverInstalled( QString ) ), this, SLOT( onFinishedInstalling( QString ) ) );
    connect( AtticaManager::instance(), SIGNAL( resolverInstallationFailed( QString ) ), this, SLOT( resolverInstallFailed( QString ) ) );

    connect( AccountManager::instance(), SIGNAL( added( Tomahawk::Accounts::Account* ) ), this, SLOT( accountAdded( Tomahawk::Accounts::Account* ) ) );
    connect( AccountManager::instance(), SIGNAL( removed( Tomahawk::Accounts::Account* ) ), this, SLOT( accountRemoved( Tomahawk::Accounts::Account* ) ) );
    connect( AccountManager::instance(), SIGNAL( stateChanged( Account* ,Accounts::Account::ConnectionState ) ), this, SLOT( accountStateChanged( Account*, Accounts::Account::ConnectionState ) ) );

    loadData();
}


void
AccountModel::atticaLoaded()
{
    m_waitingForAtticaLoaded = false;
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
#if ACCOUNTMODEL_DEBUG
    qDebug() << "All accounts:";
    foreach ( Account* acct, allAccounts )
        qDebug() << acct->accountFriendlyName() << "\t" << acct->accountId();
#endif
    foreach ( AccountFactory* fac, factories )
    {
        if ( !fac->allowUserCreation() )
            continue;

        qDebug() << "Creating factory node:" << fac->prettyName();
        m_accounts << new AccountModelNode( fac );

        // remove the accounts we are dealing with
        foreach ( Account* acct, allAccounts )
        {
            if ( AccountManager::instance()->factoryForAccount( acct ) == fac )
                allAccounts.removeAll( acct );
        }
    }

    // add all attica resolvers (installed or uninstalled)
    Attica::Content::List fromAttica = AtticaManager::instance()->resolvers();
    foreach ( const Attica::Content& content, fromAttica )
    {
        qDebug() << "Loading ATTICA ACCOUNT with content:" << content.id() << content.name();
        if ( AtticaManager::instance()->hasCustomAccountForAttica( content.id() ) )
        {
            Account* acct = AtticaManager::instance()->customAccountForAttica( content.id() );
            Q_ASSERT( acct );
            if ( acct )
            {
                m_accounts << new AccountModelNode( acct );
                const int removed = allAccounts.removeAll( acct );
#if ACCOUNTMODEL_DEBUG
                qDebug() << "Removed custom account from misc accounts list, found:" << removed;
                qDebug() << "All accounts after remove:";
                foreach ( Account* acct, allAccounts )
                    qDebug() << acct->accountFriendlyName() << "\t" << acct->accountId();    // All other accounts we haven't dealt with yet
#else
                Q_UNUSED( removed );
#endif
            }
        } else
        {
            m_accounts << new AccountModelNode( content );

            foreach ( Account* acct, AccountManager::instance()->accounts( Accounts::ResolverType ) )
            {
#if ACCOUNTMODEL_DEBUG
                qDebug() << "Found ResolverAccount" << acct->accountFriendlyName();
#endif
                if ( AtticaResolverAccount* resolver = qobject_cast< AtticaResolverAccount* >( acct ) )
                {
#if ACCOUNTMODEL_DEBUG
                    qDebug() << "Which is an attica resolver with id:" << resolver->atticaId();
#endif
                    if ( resolver->atticaId() == content.id() )
                    {
                        allAccounts.removeAll( acct );
                    }
                }
            }
        }
    }

#if ACCOUNTMODEL_DEBUG
    qDebug() << "All accounts left:";
    foreach ( Account* acct, allAccounts )
        qDebug() << acct->accountFriendlyName() << "\t" << acct->accountId();    // All other accounts we haven't dealt with yet
#endif
   foreach ( Account* acct, allAccounts )
   {
       qDebug() << "Resolver is left over:" << acct->accountFriendlyName() << acct->accountId();
//        Q_ASSERT( !qobject_cast< AtticaResolverAccount* >( acct ) ); // This should be caught above in the attica list

       if ( qobject_cast< ResolverAccount* >( acct ) && !qobject_cast< AtticaResolverAccount* >( acct ) )
           m_accounts << new AccountModelNode( qobject_cast< ResolverAccount* >( acct ) );
       else
           m_accounts << new AccountModelNode( acct );
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
                    return !acct->accountFriendlyName().isEmpty() ? acct->accountFriendlyName() : node->factory->prettyName();
                case Qt::DecorationRole:
                    return acct->icon();
                case DescriptionRole:
                    return node->factory ?
                              ( !node->factory->description().isEmpty() ? node->factory->description() : acct->description() )
                              : acct->description();
                case AuthorRole:
                    return acct->author();
                case VersionRole:
                    return acct->version();
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
        case AccountModelNode::CustomAccountType:
        {
            Q_ASSERT( node->customAccount );
            Q_ASSERT( node->factory );

            Attica::Content content = node->atticaContent;
            // This is ugly. CustomAccounts are pure Account*, but we know that
            // some might also be linked to attica resolvers (not always). If that is the case
            // they have a Attica::Content set on the node, so we use that to display some
            // extra metadata and rating
            Account* account = node->customAccount;
            if ( node->type == AccountModelNode::CustomAccountType && qobject_cast< CustomAtticaAccount* >( account ) )
                content = qobject_cast< CustomAtticaAccount* >( node->customAccount )->atticaContent();
            const bool hasAttica = !content.id().isNull();

            switch ( role )
            {
                case Qt::DisplayRole:
                    return account->accountFriendlyName();
                case Qt::DecorationRole:
                    return account->icon();
                case StateRole:
                    return ShippedWithTomahawk;
                case Qt::ToolTipRole:
                case DescriptionRole:
                    return hasAttica ? content.description() : node->factory->description();
                case CanRateRole:
                    return hasAttica;
                case AuthorRole:
                    return hasAttica ? content.author() : QString();
                case RatingRole:
                    return hasAttica ? content.rating() / 20 : 0; // rating is out of 100
                case DownloadCounterRole:
                    return hasAttica ? content.downloads() : QVariant();
                case RowTypeRole:
                    return CustomAccount;
                case AccountData:
                    return QVariant::fromValue< QObject* >( account );
                case HasConfig:
                    return account->configurationWidget() != 0;
                case AccountTypeRole:
                    return QVariant::fromValue< AccountTypes >( account->types() );
                case Qt::CheckStateRole:
                    return account->enabled() ? Qt::Checked : Qt::Unchecked;
                case ConnectionStateRole:
                    return account->connectionState();
                case UserHasRatedRole:
                    return hasAttica ? AtticaManager::instance()->userHasRated( content ) : false;
                default:
                    return QVariant();
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

        Qt::CheckState checkState = static_cast< Qt::CheckState >( value.toInt() );

        switch ( node->type )
        {
            case AccountModelNode::UniqueFactoryType:
            {
                const Qt::CheckState state = static_cast< Qt::CheckState >( value.toInt() );
                if ( node->accounts.isEmpty() )
                {
                    Q_ASSERT( state == Qt::Checked ); // How could we have a checked unique factory w/ no account??
                    // No account for this unique factory, create it
                    // Don't add it to node->accounts here, slot attached to accountmanager::accountcreated will do it for us
                    acct = node->factory->createAccount();
                    AccountManager::instance()->addAccount( acct );
                    TomahawkSettings::instance()->addAccount( acct->accountId() );
                }
                else
                {
                    Q_ASSERT( node->accounts.size() == 1 );
                    acct = node->accounts.first();
                }
                break;
            }
            case AccountModelNode::AtticaType:
            {
                // This may or may not be installed. if it's not installed yet, install it, then go ahead and enable it
                Q_ASSERT( node->atticaContent.isValid() );

                Attica::Content resolver = node->atticaContent;
                AtticaManager::ResolverState state = AtticaManager::instance()->resolverState( resolver );
                qDebug() << "Attica resolver was checked! Current state is:" << state << "and so..";
                if ( state == AtticaManager::Installed && !node->atticaAccount )
                {
                    // Something is wrong, reinstall
                    qDebug() << "Found installed state but no resolver, uninstalling first";
                    AtticaManager::instance()->uninstallResolver( resolver );
                    state = AtticaManager::Uninstalled;
                }

                // Don't install if we're unchecking. This happens if e.g. the user deletes his config file
                // and opens tomahawk
                if ( state == AtticaManager::Installed || checkState == Qt::Unchecked )
                {
                    qDebug() << "Already installed with resolver, or unchecking, just enabling/disabling";
                    acct = node->atticaAccount;
                    break;
                }
                else
                {
                    if ( m_waitingForAtticaInstall.contains( resolver.id() ) )
                    {
                        // in progress, ignore
                        return true;
                    }

                    qDebug() << "Kicked off fetch+install, now waiting";
                    m_waitingForAtticaInstall.insert( resolver.id() );

                    if ( node->atticaAccount )
                        AtticaManager::instance()->installResolverWithHandler( resolver, node->atticaAccount );
                    else
                        AtticaManager::instance()->installResolver( resolver, true );

                    return true;
                }

            }
            case AccountModelNode::ManualResolverType:
                acct = node->resolverAccount;
                break;
            case AccountModelNode::CustomAccountType:
                acct = node->customAccount;
                break;
            default:
                ;
        };

        if ( node->type == AccountModelNode::FactoryType )
        {
            tLog() << "Factory account with members:" << node->accounts << node->accounts.size();
            // Turn on or off all accounts for this factory
            foreach ( Account* acct, node->accounts )
            {
                tLog() << "Account we are toggling for factory:" << acct;
                if ( !acct )
                    continue;

                checkState == Qt::Checked ? AccountManager::instance()->enableAccount( acct )
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

#if defined(Q_OS_LINUX) && !defined(ENABLE_HEADLESS)
        if ( acct->preventEnabling() )
        {
            // Can't install from attica yet on linux, so show a warning if the user tries to turn it on.
            // TODO make a prettier display
            QMessageBox box;
            box.setWindowTitle( tr( "Manual Install Required" ) );
            box.setTextFormat( Qt::RichText );
            box.setIcon( QMessageBox::Information );
            box.setText( tr( "Unfortunately, automatic installation of this resolver is not available or disabled for your platform.<br /><br />"
            "Please use \"Install from file\" above, by fetching it from your distribution or compiling it yourself. Further instructions can be found here:<br /><br />http://www.tomahawk-player.org/resolvers/%1" ).arg( acct->accountServiceName() ) );
            box.setStandardButtons( QMessageBox::Ok );
            box.exec();
        }
#endif
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
        Attica::Content content;
        if ( node->type == AccountModelNode::AtticaType )
        {
            content = node->atticaContent;

            AtticaManager::ResolverState state = AtticaManager::instance()->resolverState( content );
            // For now only allow rating if a resolver is installed!
            if ( state != AtticaManager::Installed && state != AtticaManager::NeedsUpgrade )
                return false;
        } // Allow rating custom attica accounts regardless as user may have installed manually
        else if ( node->type == AccountModelNode::CustomAccountType && qobject_cast< CustomAtticaAccount* >( node->customAccount ) )
            content = qobject_cast< CustomAtticaAccount* >( node->customAccount )->atticaContent();

        Q_ASSERT( !content.id().isNull() );

        if ( AtticaManager::instance()->userHasRated( content ) )
            return false;

        content.setRating( value.toInt() * 20 );
        AtticaManager::instance()->uploadRating( content );

        if ( node->type == AccountModelNode::AtticaType )
            node->atticaContent = content;

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
    ResolverAccount* resolver = qobject_cast< ResolverAccount* >( account );
    AtticaResolverAccount* attica = qobject_cast< AtticaResolverAccount* >( account );
    for ( int i = 0; i < m_accounts.size(); i++ )
    {
        AccountModelNode* n = m_accounts.at( i );
        bool thisIsTheOne = false;
        qDebug() << "Checking for added account's related factory or attica:" << n->factory << attica << resolver;
        if ( attica )
            qDebug() << n->atticaContent.id() << n->atticaContent.name() << attica->atticaId();
        if ( n->factory == factory && !(resolver && !attica) ) // Specifically ignore manual resolvers, as the user added them directly
        {
            qDebug() << "Found added account with factory we already havel" << n->factory << factory;
            n->accounts << account;
            thisIsTheOne = true;
        }
        else if ( attica && n->atticaContent.id() == attica->atticaId() )
        {

            n->atticaAccount = attica;
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
        qDebug() << "Plain old manual resolver added, appending at end";
        if ( !m_waitingForAtticaLoaded )
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
    for ( int i = 0; i < m_accounts.size(); i++ )
    {
        AccountModelNode* n = m_accounts.at( i );
        if ( n->type != AccountModelNode::FactoryType )
        {
            // If this is not a non-unique factory, it has as top-level account, so find that and update it
            // For each type that this node could be, check the corresponding data
            if ( ( n->type == AccountModelNode::UniqueFactoryType && n->accounts.size() && n->accounts.first() == account ) ||
                 ( n->type == AccountModelNode::AtticaType && n->atticaAccount && n->atticaAccount == account ) ||
                 ( n->type == AccountModelNode::ManualResolverType && n->resolverAccount && n->resolverAccount == account ) ||
                 ( n->type == AccountModelNode::CustomAccountType && n->customAccount && n->customAccount == account ) )
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
            n->atticaAccount = 0;
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
AccountModel::resolverInstallFailed( const QString& resolverId )
{
    const QModelIndex idx = indexForAtticaId( resolverId );
    if ( idx.isValid() )
    {
        qDebug() << "Got failed attica install in account mode, emitting signal!";
        emit errorInstalling( idx );
    }
}


QModelIndex
AccountModel::indexForAtticaId( const QString& resolverId ) const
{
    for ( int i = 0; i < m_accounts.size(); i++ )
    {
        if ( m_accounts[ i ]->type == AccountModelNode::AtticaType && m_accounts[ i ]->atticaContent.id() == resolverId )
        {
            return index( i, 0, QModelIndex() );
        }
        else if ( m_accounts[ i ]->type == AccountModelNode::CustomAccountType && qobject_cast< CustomAtticaAccount* >( m_accounts[ i ]->customAccount ) )
        {
            const CustomAtticaAccount* atticaAcct = qobject_cast< CustomAtticaAccount* >( m_accounts[ i ]->customAccount );
            if ( atticaAcct->atticaContent().id() == resolverId )
                return index( i, 0, QModelIndex() );
        }
    }

    return QModelIndex();
}


void
AccountModel::onStartedInstalling( const QString& resolverId )
{
    const QModelIndex idx = indexForAtticaId( resolverId );
    if ( idx.isValid() )
    {
        qDebug() << "Got resolver that is beginning to install, emitting signal";
        emit startInstalling( idx );
    }
}


void
AccountModel::onFinishedInstalling( const QString& resolverId )
{
    const QModelIndex idx = indexForAtticaId( resolverId );
    if ( idx.isValid() )
    {
        qDebug() << "Got resolver that is beginning to install, emitting signal";
        emit doneInstalling( idx );
    }
}


int
AccountModel::rowCount( const QModelIndex& ) const
{
    return m_accounts.size();
}

