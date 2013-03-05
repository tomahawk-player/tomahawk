/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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


#include "ScriptCollection.h"

#include "Source.h"
#include "ExternalResolverGui.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "resolvers/ScriptCommand_AllArtists.h"
#include "resolvers/ScriptCommand_AllAlbums.h"
#include "resolvers/ScriptCommand_AllTracks.h"

#include <QPainter>

using namespace Tomahawk;


ScriptCollection::ScriptCollection( const source_ptr& source,
                                    ExternalResolver* resolver,
                                    QObject* parent )
    : Collection( source, QString( "scriptcollection:" + resolver->name() + ":" + uuid() ), parent )
    , m_trackCount( -1 ) //null value
{
    Q_ASSERT( resolver != 0 );
    qDebug() << Q_FUNC_INFO << resolver->name() << name();

    m_resolver = resolver;

    m_servicePrettyName = m_resolver->name();

    ExternalResolverGui* gResolver = qobject_cast< ExternalResolverGui* >( m_resolver );
    if ( gResolver )
    {
        m_icon = gResolver->icon();
    }
}


ScriptCollection::~ScriptCollection()
{

}


void
ScriptCollection::setServiceName( const QString& name )
{
    m_servicePrettyName = name;
}


QString
ScriptCollection::prettyName() const
{
    return tr( "%1 Collection",
               "Name of a collection based on a resolver, e.g. Subsonic Collection" )
               .arg( m_servicePrettyName );
}


QString
ScriptCollection::itemName() const
{
    return m_servicePrettyName;
}


void
ScriptCollection::setIcon( const QIcon& icon )
{
    m_icon = icon;
    emit changed();
}


QIcon
ScriptCollection::icon() const
{
    return m_icon;
}


QPixmap
ScriptCollection::bigIcon() const
{
    QPixmap big = Collection::bigIcon();
    QPixmap base = icon().pixmap( big.size() );

    if ( !source()->isLocal() )
    {
        big = big.scaled( TomahawkUtils::defaultIconSize(),
                          Qt::KeepAspectRatio,
                          Qt::SmoothTransformation );

        QPainter painter( &base );
        painter.drawPixmap( base.width() - big.width(),
                            base.height() - big.height(),
                            big.width(),
                            big.height(),
                            big );
        painter.end();
    }

    return base;
}


void
ScriptCollection::setDescription( const QString& text )
{
    m_description = text;
}


QString
ScriptCollection::description() const
{
    return m_description;
}


Tomahawk::ArtistsRequest*
ScriptCollection::requestArtists()
{
    Tomahawk::collection_ptr thisCollection = m_resolver->collections().value( name() );
    if ( thisCollection->name() != this->name() )
        return 0;

    Tomahawk::ArtistsRequest* cmd = new ScriptCommand_AllArtists( thisCollection );

    return cmd;
}


Tomahawk::AlbumsRequest*
ScriptCollection::requestAlbums( const Tomahawk::artist_ptr& artist )
{
    Tomahawk::collection_ptr thisCollection = m_resolver->collections().value( name() );
    if ( thisCollection->name() != this->name() )
        return 0;

    Tomahawk::AlbumsRequest* cmd = new ScriptCommand_AllAlbums( thisCollection, artist );

    return cmd;
}


Tomahawk::TracksRequest*
ScriptCollection::requestTracks( const Tomahawk::album_ptr& album )
{
    Tomahawk::collection_ptr thisCollection = m_resolver->collections().value( name() );
    if ( thisCollection->name() != this->name() )
        return 0;

    Tomahawk::TracksRequest* cmd = new ScriptCommand_AllTracks( thisCollection, album );

    return cmd;
}


void
ScriptCollection::setTrackCount( int count )
{
    m_trackCount = count;
}


int
ScriptCollection::trackCount() const
{
    return m_trackCount;
}
