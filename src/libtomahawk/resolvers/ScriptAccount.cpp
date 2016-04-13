/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011,      Leo Franchi <lfranchi@kde.org>
 *   Copyright 2014,      Dominik Schmidt <domme@tomahawk-player.org>
 *   Copyright 2015-2016, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "ScriptAccount.h"

#include "ScriptObject.h"
#include "../utils/Logger.h"
#include "../Typedefs.h"

#include "plugins/ScriptCollectionFactory.h"
#include "plugins/ScriptInfoPluginFactory.h"

// TODO: register factory methods instead of hardcoding all plugin types in here
#include "../utils/LinkGenerator.h"
#include "ScriptLinkGeneratorPlugin.h"
#include "ScriptInfoPlugin.h"

#include "../Artist.h"
#include "../Album.h"
#include "../Result.h"
#include "../Track.h"

#include <QTime>


using namespace Tomahawk;


ScriptAccount::ScriptAccount( const QString& name )
    : QObject()
    , m_name( name )
    , m_stopped( true )
    , m_collectionFactory( new ScriptCollectionFactory() )
    , m_infoPluginFactory( new ScriptInfoPluginFactory() )
{
}


ScriptAccount::~ScriptAccount()
{
    delete m_collectionFactory;
    delete m_infoPluginFactory;
}


void
ScriptAccount::start()
{
    m_stopped = false;

    m_collectionFactory->addAllPlugins();
    m_infoPluginFactory->addAllPlugins();
}


void
ScriptAccount::stop()
{
    m_stopped = true;

    m_collectionFactory->removeAllPlugins();
    m_infoPluginFactory->removeAllPlugins();
}


bool
ScriptAccount::isStopped()
{
    return m_stopped;
}


QString
ScriptAccount::name() const
{
    return m_name;
}


void
ScriptAccount::setIcon(const QPixmap& icon)
{
    m_icon = icon;
}


QPixmap
ScriptAccount::icon() const
{
    return m_icon;
}


void
ScriptAccount::setFilePath( const QString& filePath )
{
    m_filePath = filePath;
}


QString
ScriptAccount::filePath() const
{
    return m_filePath;
}


static QString
requestIdGenerator()
{
    static int requestCounter = 0;
    return QString::number( ++requestCounter );

}


ScriptJob*
ScriptAccount::invoke( const scriptobject_ptr& scriptObject, const QString& methodName, const QVariantMap& arguments )
{
    QString requestId = requestIdGenerator();

    ScriptJob* job = new ScriptJob( requestId, scriptObject, methodName, arguments );

    connect( job, SIGNAL( destroyed( QString ) ), SLOT( onJobDeleted( QString ) ) );
    m_jobs.insert( requestId, job );

    return job;
}


void
ScriptAccount::reportScriptJobResult( const QVariantMap& result )
{
    //tLog() << Q_FUNC_INFO << result;
    const QString requestId = result[ "requestId" ].toString();
    Q_ASSERT( !requestId.isEmpty() );

    ScriptJob* job = m_jobs.value( requestId );
    Q_ASSERT( job );

    // got a successful job result
    if ( result[ "error" ].isNull() )
    {
	if ( result[ "data" ].type() == QVariant::Map )
	{
	    const QVariantMap data = result[ "data" ].toMap();
	    job->reportResultsMap( data );
	}
	else
	{
	    job->reportResults( result[ "data" ] );
	}
    }
    else
    {
        job->reportFailure( result[ "error" ].toString() );
    }
}


void
ScriptAccount::registerScriptPlugin( const QString& type, const QString& objectId )
{
    scriptobject_ptr object = m_objects.value( objectId );
    if( !object )
    {
        object = scriptobject_ptr( new ScriptObject( objectId, this ), &ScriptObject::deleteLater );
        object->setWeakRef( object.toWeakRef() );
        connect( object.data(), SIGNAL( destroyed( QObject* ) ), SLOT( onScriptObjectDeleted() ) );
        m_objects.insert( objectId, object );
    }

    scriptPluginFactory( type, object );
}



void
ScriptAccount::unregisterScriptPlugin( const QString& type, const QString& objectId )
{
    scriptobject_ptr object = m_objects.value( objectId );
    if( !object )
    {
        tLog() << "ScriptAccount" << name() << "tried to unregister plugin that was not registered";
        return;
    }

    if ( type == "collection" )
    {
        m_collectionFactory->unregisterPlugin( object );
    }
    else if ( type == "infoPlugin" )
    {
        m_infoPluginFactory->unregisterPlugin( object );
    }
    else if( type == "linkParser" )
    {
        // TODO
    }
    else
    {
        tLog() << "This plugin type is not handled by Tomahawk or simply cannot be removed yet";
        Q_ASSERT( false );
    }
}


void
ScriptAccount::onScriptObjectDeleted()
{
    foreach( const scriptobject_ptr& object, m_objects.values() )
    {
        if ( object.isNull() )
        {
            m_objects.remove( m_objects.key( object ) );
            break;
        }
    }
}


void
ScriptAccount::scriptPluginFactory( const QString& type, const scriptobject_ptr& object )
{
    if ( type == "linkGenerator" )
    {
        ScriptLinkGeneratorPlugin* lgp = new ScriptLinkGeneratorPlugin( object );
        Utils::LinkGenerator::instance()->addPlugin( lgp );
    }
    else if( type == "linkParser" )
    {
        tLog() << "Plugin registered linkParser, which is not implemented yet. UrlLookup won't work";
    }
    else if ( type == "infoPlugin" )
    {
        m_infoPluginFactory->registerPlugin( object, this );
    }
    else if( type == "collection" )
    {
        m_collectionFactory->registerPlugin( object, this );
    }
    else
    {
        tLog() << "This plugin type is not handled by Tomahawk";
        Q_ASSERT( false );
    }
}


void
ScriptAccount::showDebugger()
{
}


void
ScriptAccount::onJobDeleted( const QString& jobId )
{
    m_jobs.remove( jobId );
}


QList< Tomahawk::artist_ptr >
ScriptAccount::parseArtistVariantList( const QVariantList& artistList )
{
    QList< Tomahawk::artist_ptr > artists;

    QString artist;
    foreach( const QVariant& a, artistList )
    {
        artist = a.toString().trimmed();
        if ( artist.isEmpty() )
            continue;

        artists << Tomahawk::Artist::get( artist );
    }

    return artists;
}


QList< Tomahawk::album_ptr >
ScriptAccount::parseAlbumVariantList( const QVariantList& albumList )
{
    QList< Tomahawk::album_ptr > albums;

    QString artistString;
    QString albumString;
    foreach( const QVariant& av, albumList )
    {
        QVariantMap m = av.toMap();

        artistString = m.value( "artist" ).toString().trimmed();
        albumString = m.value( "album" ).toString().trimmed();
        if ( artistString.isEmpty() || albumString.isEmpty() )
            continue;

        albums << Tomahawk::Album::get( Tomahawk::Artist::get( artistString ), albumString );
    }

    return albums;
}


QList< Tomahawk::result_ptr >
ScriptAccount::parseResultVariantList( const QVariantList& reslist )
{
    QList< Tomahawk::result_ptr > results;


    foreach( const QVariant& rv, reslist )
    {
        QVariantMap m = rv.toMap();

        const QString artistString = m.value("artist").toString().trimmed();
        const QString trackString = m.value("track").toString().trimmed();

        if ( artistString.isEmpty() || trackString.isEmpty() )
        {
            tLog() << Q_FUNC_INFO << "Could not parse Track" << m;
            continue;
        }

        int duration = m.value( "duration", 0 ).toInt();
        if ( duration <= 0 && m.contains( "durationString" ) )
        {
            QTime time = QTime::fromString( m.value( "durationString" ).toString(), "hh:mm:ss" );
            duration = time.secsTo( QTime( 0, 0 ) ) * -1;
        }

        Tomahawk::track_ptr track = Tomahawk::Track::get( artistString,
                                                          trackString,
                                                          m.value( "album" ).toString(),
                                                          m.value( "albumArtist" ).toString(),
                                                          duration,
                                                          QString(),
                                                          m.value( "albumpos" ).toUInt(),
                                                          m.value( "discnumber" ).toUInt() );
        if ( !track )
            continue;

        Tomahawk::result_ptr rp = Tomahawk::Result::get( m.value( "url" ).toString(), track );
        if ( !rp )
            continue;

        rp->setBitrate( m.value( "bitrate" ).toUInt() );
        rp->setSize( m.value( "size" ).toUInt() );
        rp->setRID( uuid() );
        rp->setPreview( m.value( "preview" ).toBool() );
        rp->setPurchaseUrl( m.value( "purchaseUrl" ).toString() );
        rp->setLinkUrl( m.value( "linkUrl" ).toString() );
//FIXME?        rp->setScore( m.value( "score" ).toFloat() );
        rp->setChecked( m.value( "checked" ).toBool() );

        //FIXME
        if ( m.contains( "year" ) )
        {
            QVariantMap attr;
            attr[ "releaseyear" ] = m.value( "year" );
//            rp->track()->setAttributes( attr );
        }


        QString mimetype = m.value( "mimetype" ).toString();
        if ( mimetype.isEmpty() )
        {
            mimetype = TomahawkUtils::extensionToMimetype( m.value( "extension" ).toString() );
        }
        Q_ASSERT( !mimetype.isEmpty() );

        if ( !mimetype.isEmpty() )
        {
            rp->setMimetype( mimetype );
        }

        rp->setFriendlySource( name() );

        QList<DownloadFormat> fl;
        foreach ( const QVariant& foo, m.value( "downloadUrls" ).toList() )
        {
            QVariantMap downloadUrl = foo.toMap();
            tLog() << "downloadUrl:" << downloadUrl.value( "url" ).toUrl() << "drm:" << downloadUrl.value( "drm" ).toBool() << "extension:" << downloadUrl.value( "extension").toString().toLower() << "bitrate:" << downloadUrl.value( "bitrate" ).toInt();

            DownloadFormat format;
            format.url = downloadUrl.value( "url" ).toUrl();
            format.extension = downloadUrl.value( "extension").toString().toLower();
            format.mimetype = TomahawkUtils::extensionToMimetype( format.extension );

            fl << format;
        }
        rp->setDownloadFormats( fl );

        results << rp;
    }

    return results;
}


ScriptJob*
ScriptAccount::resolve( const scriptobject_ptr& scriptObject, const query_ptr& query, const QString& resolveType )
{
    ScriptJob* job = nullptr;
    if ( !query->isFullTextQuery() )
    {
        QVariantMap arguments;
        arguments["artist"] = query->queryTrack()->artist();
        arguments["album"] = query->queryTrack()->album();
        arguments["track"] = query->queryTrack()->track();
        arguments["type"] = resolveType;

        job = scriptObject->invoke( "resolve", arguments );
    }
    else
    {
        QVariantMap arguments;
        arguments["query"] = query->fullTextQuery();
        arguments["type"] = resolveType;
        job = scriptObject->invoke( "search", arguments );
    }

    job->setProperty( "qid", query->id() );

    return job;
}
