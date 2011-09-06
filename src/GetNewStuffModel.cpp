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
    m_clicked = false;

    if ( AtticaManager::instance()->resolversLoaded() )
        m_contentList = AtticaManager::instance()->resolvers();

    connect( AtticaManager::instance(), SIGNAL( resolversReloaded( Attica::Content::List ) ), this, SLOT( resolversReloaded( Attica::Content::List ) ) );

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


QVariant
GetNewStuffModel::data( const QModelIndex& index, int role ) const
{
    if ( !index.isValid() || !hasIndex( index.row(), index.column(), index.parent() ) )
        return QVariant();

    Attica::Content resolver = m_contentList[ index.row() ];
    // TODO use attica
    switch ( role )
    {
        case Qt::DisplayRole:
            return resolver.name();
        case Qt::DecorationRole:
            return QVariant::fromValue< QPixmap >( QPixmap( RESPATH "images/delegate-add.png" ) );
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
            return m_clicked ? Installed : Uninstalled;
    }
    return QVariant();
}

int
GetNewStuffModel::rowCount( const QModelIndex& parent ) const
{
    return m_contentList.count();
}

bool
GetNewStuffModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
    // the install/uninstall button was clicked
    m_clicked = !m_clicked;
    emit dataChanged( index, index );

    return true;
}
