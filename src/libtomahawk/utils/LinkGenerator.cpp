/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>
 *   Copyright (C) 2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright (C) 2011-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright (C) 2013, Uwe L. Korn <uwelk@xhochy.com>
 *   Copyright (C) 2013, Teo Mrnjavac <teo@kde.org>
 *   Copyright (C) 2014, Dominik Schmidt <domme@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
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

#include "LinkGenerator.h"

#include "TomahawkUtils.h"
#include "Logger.h"
#include "ShortLinkHelper.h"

#include "../playlist/dynamic/GeneratorInterface.h"
#include "../playlist/dynamic/DynamicPlaylist.h"
#include "../Track.h"
#include "../Artist.h"
#include "../Album.h"
#include "../resolvers/ScriptJob.h"

#include <echonest/Playlist.h>

#include <QClipboard>
#include <QApplication>
#include <QMetaObject>


using namespace Tomahawk;
using namespace Tomahawk::Utils;


// HACK: to prepare proper scripting
class SyncScriptJob : public ScriptJob
{
Q_OBJECT

public:
    SyncScriptJob( const QVariantMap& resultData ) : ScriptJob( nullptr, "nomethod")
    {
        m_data = resultData;
    }

    void start() override
    {
        QMetaObject::invokeMethod( this, "reportResults", Qt::QueuedConnection, Q_ARG( QVariantMap, m_data ) );
    }
};
#include "LinkGenerator.moc"

LinkGenerator* LinkGenerator::s_instance = 0;


LinkGenerator*
LinkGenerator::instance()
{
    if ( !s_instance )
        s_instance = new LinkGenerator;

    return s_instance;
}


LinkGenerator::LinkGenerator( QObject* parent )
    : QObject( parent )
{
}


LinkGenerator::~LinkGenerator()
{
}


QString
LinkGenerator::hostname() const
{
    return QString( "http://toma.hk" );
}


ScriptJob*
LinkGenerator::openLink( const query_ptr& query ) const
{
    QString title = query->track()->track();
    QString artist = query->track()->artist();
    QString album = query->track()->album();

    return openLink( title, artist, album );
}


ScriptJob*
LinkGenerator::openLink( const QString& title, const QString& artist, const QString& album ) const
{
    QUrl link( QString( "%1/open/track/" ).arg( hostname() ) );

    if ( !artist.isEmpty() )
       TomahawkUtils::urlAddQueryItem( link, "artist", artist );
    if ( !title.isEmpty() )
        TomahawkUtils::urlAddQueryItem( link, "title", title );
    if ( !album.isEmpty() )
        TomahawkUtils::urlAddQueryItem( link, "album", album );

    QVariantMap data;
    data[ "url" ] = link;

    return new SyncScriptJob( data );
}


ScriptJob*
LinkGenerator::openLink( const Tomahawk::dynplaylist_ptr& playlist ) const
{
    QUrl link( QString( "%1/%2/create/" ).arg( hostname() ).arg( playlist->mode() == OnDemand ? "station" : "autoplaylist" ) );

    if ( playlist->generator()->type() != "echonest" )
    {
        tLog() << "Only echonest generators are supported";
        return nullptr;
    }

    TomahawkUtils::urlAddQueryItem( link, "type", "echonest" );
    TomahawkUtils::urlAddQueryItem( link, "title", playlist->title() );

    QList< dyncontrol_ptr > controls = playlist->generator()->controls();
    foreach ( const dyncontrol_ptr& c, controls )
    {
        if ( c->selectedType() == "Artist" )
        {
            if ( c->match().toInt() == Echonest::DynamicPlaylist::ArtistType )
                TomahawkUtils::urlAddQueryItem( link, "artist_limitto", c->input() );
            else
                TomahawkUtils::urlAddQueryItem( link, "artist", c->input() );
        }
        else if ( c->selectedType() == "Artist Description" )
        {
            TomahawkUtils::urlAddQueryItem( link, "description", c->input() );
        }
        else
        {
            QString name = c->selectedType().toLower().replace( " ", "_" );
            Echonest::DynamicPlaylist::PlaylistParam p = static_cast< Echonest::DynamicPlaylist::PlaylistParam >( c->match().toInt() );
            // if it is a max, set that too
            if ( p == Echonest::DynamicPlaylist::MaxTempo || p == Echonest::DynamicPlaylist::MaxDuration || p == Echonest::DynamicPlaylist::MaxLoudness
               || p == Echonest::DynamicPlaylist::MaxDanceability || p == Echonest::DynamicPlaylist::MaxEnergy || p == Echonest::DynamicPlaylist::ArtistMaxFamiliarity
               || p == Echonest::DynamicPlaylist::ArtistMaxHotttnesss || p == Echonest::DynamicPlaylist::SongMaxHotttnesss || p == Echonest::DynamicPlaylist::ArtistMaxLatitude
               || p == Echonest::DynamicPlaylist::ArtistMaxLongitude )
                name += "_max";

            TomahawkUtils::urlAddQueryItem( link, name, c->input() );
        }
    }

    QVariantMap data;
    data[ "url" ] = link;

    return new SyncScriptJob( data );
}


ScriptJob*
LinkGenerator::openLink( const Tomahawk::artist_ptr& artist ) const
{
    QVariantMap data;
    data[ "url" ] = QString( "%1/artist/%2" ).arg( hostname() ).arg( artist->name() );
    return new SyncScriptJob( data );
}


ScriptJob*
LinkGenerator::openLink( const Tomahawk::album_ptr& album ) const
{
    QVariantMap data;
    data[ "url" ] = QUrl::fromUserInput( QString( "%1/album/%2/%3" ).arg( hostname() ).arg( album->artist().isNull() ? QString() : album->artist()->name() ).arg( album->name() ) );;
    return new SyncScriptJob( data );
}


void
LinkGenerator::copyScriptJobResultToClipboard( const QVariantMap& data )
{
    m_clipboardLongUrl = data[ "url" ].toUrl();
    copyToClipboardReady( m_clipboardLongUrl, m_clipboardLongUrl );

    sender()->deleteLater();
}


void
LinkGenerator::copyScriptJobResultToClipboardShortened( const QVariantMap& data )
{
    m_clipboardLongUrl = data[ "url" ].toUrl();

    Tomahawk::Utils::ShortLinkHelper* slh = new Tomahawk::Utils::ShortLinkHelper();
    connect( slh, SIGNAL( shortLinkReady( QUrl, QUrl, QVariant ) ),
             SLOT( copyToClipboardReady( QUrl, QUrl, QVariant ) ) );
    connect( slh, SIGNAL( done() ),
             slh, SLOT( deleteLater() ),
             Qt::QueuedConnection );
    slh->shortenLink( m_clipboardLongUrl );

    sender()->deleteLater();
}


void
LinkGenerator::copyToClipboardReady( const QUrl& longUrl, const QUrl& shortUrl, const QVariant& )
{
    // Copy resulting url to clipboard
    if ( m_clipboardLongUrl == longUrl )
    {
        QClipboard* cb = QApplication::clipboard();

        QByteArray data = TomahawkUtils::percentEncode( shortUrl.isEmpty() ? longUrl : shortUrl );
        cb->setText( data );

        m_clipboardLongUrl.clear();
    }
}
