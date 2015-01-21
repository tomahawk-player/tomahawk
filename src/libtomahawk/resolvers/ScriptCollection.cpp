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
#include "utils/TomahawkUtilsGui.h"
#include "utils/NetworkAccessManager.h"
#include "utils/Logger.h"
#include "resolvers/ScriptCommand_AllArtists.h"
#include "resolvers/ScriptCommand_AllAlbums.h"
#include "resolvers/ScriptCommand_AllTracks.h"
#include "ScriptAccount.h"

#include <QImageReader>
#include <QPainter>
#include <QFileInfo>


using namespace Tomahawk;


ScriptCollection::ScriptCollection( const scriptobject_ptr& scriptObject,
                                    const source_ptr& source,
                                    ScriptAccount* scriptAccount,
                                    QObject* parent )
    : Collection( source, QString( "scriptcollection:" + scriptAccount->name() + ":" + uuid() ), parent )
    , ScriptPlugin( scriptObject )
    , m_scriptAccount( scriptAccount )
    , m_trackCount( -1 ) //null value
    , m_isOnline( true )
{
    Q_ASSERT( scriptAccount );
    qDebug() << Q_FUNC_INFO << scriptAccount->name() << Collection::name();

    m_servicePrettyName = scriptAccount->name();
}


ScriptCollection::~ScriptCollection()
{
}


ScriptAccount*
ScriptCollection::scriptAccount() const
{
    return m_scriptAccount;
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
               "Name of a collection based on a script pluginsc, e.g. Subsonic Collection" )
               .arg( m_servicePrettyName );
}


QString
ScriptCollection::itemName() const
{
    return m_servicePrettyName;
}


bool ScriptCollection::isOnline() const
{
    return m_isOnline;
}


void
ScriptCollection::setOnline( bool isOnline )
{
    m_isOnline = isOnline;

    if ( isOnline )
    {
        emit online();
    }
    else
    {
        emit offline();
    }


    emit changed();
}


void
ScriptCollection::setIcon( const QPixmap& icon )
{
    m_icon = icon;
    emit changed();
}


QPixmap
ScriptCollection::icon( const QSize& size ) const
{
    if ( !size.isEmpty() && !m_icon.isNull() )
        return m_icon.scaled( size, Qt::KeepAspectRatio, Qt::SmoothTransformation );

    return m_icon;
}


QPixmap
ScriptCollection::bigIcon() const
{
    QPixmap big = Collection::bigIcon();
    QPixmap base = icon( big.size() );

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
    Tomahawk::ArtistsRequest* cmd = new ScriptCommand_AllArtists( weakRef().toStrongRef() );

    return cmd;
}


Tomahawk::AlbumsRequest*
ScriptCollection::requestAlbums( const Tomahawk::artist_ptr& artist )
{
    Tomahawk::AlbumsRequest* cmd = new ScriptCommand_AllAlbums( weakRef().toStrongRef(), artist );

    return cmd;
}


Tomahawk::TracksRequest*
ScriptCollection::requestTracks( const Tomahawk::album_ptr& album )
{
    Tomahawk::TracksRequest* cmd = new ScriptCommand_AllTracks( weakRef().toStrongRef(), album );

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


QVariantMap
ScriptCollection::readMetaData()
{
    return scriptObject()->syncInvoke( "collection" ).toMap();
}


void ScriptCollection::parseMetaData()
{
    return parseMetaData( readMetaData() );
}


void
ScriptCollection::parseMetaData( const QVariantMap& metadata )
{
    tLog() << Q_FUNC_INFO;

    const QString prettyname = metadata.value( "prettyname" ).toString();
    const QString desc = metadata.value( "description" ).toString();

    setServiceName( prettyname );
    setDescription( desc );

    if ( metadata.contains( "trackcount" ) ) //a resolver might not expose this
    {
        bool ok = false;
        int trackCount = metadata.value( "trackcount" ).toInt( &ok );
        if ( ok )
            setTrackCount( trackCount );
    }

    if ( metadata.contains( "iconfile" ) )
    {
        QString iconPath = QFileInfo( scriptAccount()->filePath() ).path() + "/"
                            + metadata.value( "iconfile" ).toString();

        QPixmap iconPixmap;
        bool ok = iconPixmap.load( iconPath );
        if ( ok && !iconPixmap.isNull() )
            setIcon( iconPixmap );

        fetchIcon( metadata.value( "iconurl" ).toString() );
    }

    if ( metadata.contains( "capabilities" ) )
    {
        QVariantList list = metadata[ "capabilities" ].toList();

        foreach( const QVariant& type, list )
        {
            bool ok;
            int intType = type.toInt( &ok );
            if ( ok )
            {
                m_browseCapabilities << static_cast< BrowseCapability >( intType );
            }
        }

    }
    else
    {
        m_browseCapabilities << CapabilityBrowseArtists;
    }
}


void
ScriptCollection::fetchIcon( const QString& iconUrlString )
{
    if ( !iconUrlString.isEmpty() )
    {
        QUrl iconUrl = QUrl::fromEncoded( iconUrlString.toLatin1() );
        if ( iconUrl.isValid() )
        {
            QNetworkRequest req( iconUrl );
            tDebug() << "Creating a QNetworkReply with url:" << req.url().toString();
            QNetworkReply* reply = Tomahawk::Utils::nam()->get( req );

            connect( reply, SIGNAL( finished() ),
                        this, SLOT( onIconFetched() ) );
        }
    }
}


void
ScriptCollection::onIconFetched()
{
    QNetworkReply* reply = qobject_cast< QNetworkReply* >( sender() );
    if ( reply != 0 )
    {
        if( reply->error() == QNetworkReply::NoError )
        {
            QImageReader imageReader( reply );
            setIcon( QPixmap::fromImageReader( &imageReader ) );
        }

        reply->deleteLater();
    }
}
