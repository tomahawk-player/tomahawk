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

#include "GetNewStuffModel.h"

#include "utils/tomahawkutils.h"
#include "utils/logger.h"
#include "AtticaManager.h"
#include "accounts/AccountManager.h"

#include <QPixmap>
#include <QUrl>

using namespace Tomahawk;
using namespace Accounts;

GetNewStuffModel::GetNewStuffModel( QObject* parent )
    : QAbstractListModel ( parent )
{
    connect( AtticaManager::instance(), SIGNAL( resolversReloaded( Attica::Content::List ) ), this, SLOT( resolversReloaded( Attica::Content::List ) ) );
    connect( AtticaManager::instance(), SIGNAL( resolverStateChanged( QString ) ), this, SLOT( resolverStateChanged( QString ) ) );

    loadData();
}

GetNewStuffModel::~GetNewStuffModel()
{
}

void
GetNewStuffModel::loadData()
{
    foreach ( const QVariant& content, m_contentList )
    {
        if ( !isAttica( content ) )
        {
            AccountItem* item = content.value< GetNewStuffModel::AccountItem* >();
            delete item;
        }
    }
    m_contentList.clear();

    Attica::Content::List fromAttica = AtticaManager::instance()->resolvers();
    foreach ( const Attica::Content& content, fromAttica )
        m_contentList.append( QVariant::fromValue< Attica::Content >( content ) );

    QList< AccountFactory* > factories = AccountManager::instance()->factories();
    QList< Account* > allAccounts = AccountManager::instance()->accounts();
    foreach ( AccountFactory* fac, factories )
    {
        if ( !fac->allowUserCreation() )
            continue;

        AccountItem* item = new AccountItem;
        item->factory = fac;

        foreach ( Account* acct, allAccounts )
        {
            if ( AccountManager::instance()->factoryForAccount( acct ) == fac )
            {
                item->alreadyExists = true;
                break;
            }
            else
                item->alreadyExists = false;
        }

        m_contentList.append( QVariant::fromValue< GetNewStuffModel::AccountItem* >( item ) );
    }
}


void
GetNewStuffModel::resolversReloaded( const Attica::Content::List& resolvers )
{
    beginResetModel();
    loadData();
    endResetModel();
}

void
GetNewStuffModel::resolverStateChanged( const QString& resolverId )
{
    for ( int i = 0; i < m_contentList.count(); i++ )
    {
        if ( !isAttica( m_contentList.at( i ) ) )
            continue;

        const Attica::Content resolver = atticaFromItem( m_contentList.at( i ) );
        if ( resolver.id() == resolverId )
        {
            QModelIndex idx = index( i, 0, QModelIndex() );
            emit dataChanged( idx, idx );
        }
    }
}


QVariant
GetNewStuffModel::data( const QModelIndex& index, int role ) const
{
    if ( !index.isValid() || !hasIndex( index.row(), index.column(), index.parent() ) )
        return QVariant();

    if ( isAttica( m_contentList.at( index.row() ) ) )
    {
        const Attica::Content resolver = atticaFromItem( m_contentList.at( index.row() ) );
        switch ( role )
        {
            case Qt::DisplayRole:
                return resolver.name();
            case Qt::DecorationRole:
                return QVariant::fromValue< QPixmap >( AtticaManager::instance()->iconForResolver( resolver ) );
            case DownloadUrlRole:
                // TODO
                return QUrl();
            case RatingRole:
                return resolver.rating() / 20; // rating is out of 100
            case DownloadCounterRole:
                return resolver.downloads();
            case VersionRole:
                return resolver.version();
            case DescriptionRole:
                return resolver.description();
            case TypeRole:
                return AtticaType;
            case AuthorRole:
                return resolver.author();
            case StateRole:
                return (int)AtticaManager::instance()->resolverState( resolver );
            case UserHasRatedRole:
                return AtticaManager::instance()->userHasRated( resolver );
        }
    }
    else
    {
        // Account, not from Attica
        AccountItem* item = accountFromItem( m_contentList.at( index.row() ) );
        Q_ASSERT( item );
        switch ( role )
        {
            case Qt::DisplayRole:
                return item->factory->prettyName();
            case Qt::DecorationRole:
                return QVariant::fromValue< QPixmap >( item->factory->icon() );
            case RatingRole:
                // TODO
                return 3;
//                 return resolver.rating() / 20; // rating is out of 100
            case DownloadCounterRole:
                // TODO
                return 10;
//                 return resolver.downloads();
            case VersionRole:
                return "1.0";
//                 return resolver.version();
            case DescriptionRole:
                return item->factory->description();
            case TypeRole:
                return AccountType;
            case AuthorRole:
                return "Tomahawk Developers";
            case StateRole:
            {
                GetNewStuffModel::ItemState state = Uninstalled;
                if ( item->factory->isUnique() && item->alreadyExists )
                    state = Installed;
                else if ( !item->factory->isUnique() && item->alreadyExists )
                    state = CanInstallMore;
                else if ( !item->alreadyExists )
                    state = Uninstalled;
                return (int)state;
            }
            case UserHasRatedRole:
                // TODO
                return true;
//                 return AtticaManager::instance()->userHasRated( resolver );
        }
    }

    return QVariant();
}

int
GetNewStuffModel::rowCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );
    return m_contentList.count();
}

bool
GetNewStuffModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
    Q_UNUSED( value );
    if ( !hasIndex( index.row(), index.column(), index.parent() ) )
        return false;

    if ( isAttica( m_contentList.at( index.row() ) ) )
    {
        Attica::Content resolver = atticaFromItem( m_contentList.at( index.row() ) );
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
        } else if ( role == RatingRole )
        {
            // For now only allow rating if a resolver is installed!
            if ( state != AtticaManager::Installed && state != AtticaManager::NeedsUpgrade )
                return false;
            if ( AtticaManager::instance()->userHasRated( resolver ) )
                return false;
            resolver.setRating( value.toInt() * 20 );
            m_contentList[ index.row() ] = QVariant::fromValue< Attica::Content >( resolver );
            AtticaManager::instance()->uploadRating( resolver );
        }
        emit dataChanged( index, index );
    }
    else
    {
        AccountItem* item = accountFromItem( m_contentList.at( index.row() ) );
        if ( role == Qt::EditRole )
        {
            // TODO
        }
        else if ( role == RatingRole )
        {
            // TODO
        }
    }

    return true;
}


bool
GetNewStuffModel::isAttica( const QVariant& item ) const
{
    return qstrcmp( item.typeName(),"Attica::Content" ) == 0;
}


GetNewStuffModel::AccountItem*
GetNewStuffModel::accountFromItem( const QVariant& item ) const
{
    Q_ASSERT( !isAttica( item ) );

    return item.value< GetNewStuffModel::AccountItem* >();
}


Attica::Content
GetNewStuffModel::atticaFromItem( const QVariant& item ) const
{
    Q_ASSERT( isAttica( item ) );

    return item.value< Attica::Content >();

}
