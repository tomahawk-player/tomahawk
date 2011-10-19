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

#include <QPixmap>
#include <QUrl>
#include "AtticaManager.h"

GetNewStuffModel::GetNewStuffModel( QObject* parent )
    : QAbstractListModel ( parent )
{

    if ( AtticaManager::instance()->resolversLoaded() )
        m_contentList = AtticaManager::instance()->resolvers();

    connect( AtticaManager::instance(), SIGNAL( resolversReloaded( Attica::Content::List ) ), this, SLOT( resolversReloaded( Attica::Content::List ) ) );
    connect( AtticaManager::instance(), SIGNAL( resolverStateChanged( QString ) ), this, SLOT( resolverStateChanged( QString ) ) );

}

GetNewStuffModel::~GetNewStuffModel()
{
}

void
GetNewStuffModel::resolversReloaded( const Attica::Content::List& resolvers )
{
    beginResetModel();
    m_contentList = resolvers;
    endResetModel();
}

void
GetNewStuffModel::resolverStateChanged( const QString& resolverId )
{
    for ( int i = 0; i < m_contentList.count(); i++ )
    {
        const Attica::Content resolver = m_contentList[ i ];
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

    Attica::Content resolver = m_contentList[ index.row() ];
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
            return ResolverType;
        case AuthorRole:
            return resolver.author();
        case StateRole:
            return (int)AtticaManager::instance()->resolverState( resolver );
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


    Attica::Content resolver = m_contentList[ index.row() ];
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
        if ( AtticaManager::userHasRated( resolver ) )
            return false;
        m_contentList[ index.row() ].setRating( value.toInt() * 20 );
        AtticaManager::instance()->uploadRating( m_contentList[ index.row() ] );
    }
    emit dataChanged( index, index );

    return true;
}
