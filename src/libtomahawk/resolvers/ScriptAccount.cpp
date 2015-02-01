/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>
 *   Copyright (C) 2014  Dominik Schmidt <domme@tomahawk-player.org>
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

// TODO:
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
    tLog() << Q_FUNC_INFO << result;
    const QString requestId = result[ "requestId" ].toString();
    Q_ASSERT( !requestId.isEmpty() );

    ScriptJob* job = m_jobs.value( requestId );
    Q_ASSERT( job );

    // got a successful job result
    if ( result[ "error"].isNull() )
    {
        const QVariantMap data = result[ "data" ].toMap();

        job->reportResults( data );
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
ScriptAccount::onJobDeleted( const QString& jobId )
{
    m_jobs.remove( jobId );
}


QList< Tomahawk::result_ptr >
ScriptAccount::parseResultVariantList( const QVariantList& reslist )
{
    QList< Tomahawk::result_ptr > results;

    foreach( const QVariant& rv, reslist )
    {
        QVariantMap m = rv.toMap();
        // TODO we need to handle preview urls separately. they should never trump a real url, and we need to display
        // the purchaseUrl for the user to upgrade to a full stream.
        if ( m.value( "preview" ).toBool() == true )
            continue;

        int duration = m.value( "duration", 0 ).toInt();
        if ( duration <= 0 && m.contains( "durationString" ) )
        {
            QTime time = QTime::fromString( m.value( "durationString" ).toString(), "hh:mm:ss" );
            duration = time.secsTo( QTime( 0, 0 ) ) * -1;
        }

        Tomahawk::track_ptr track = Tomahawk::Track::get( m.value( "artist" ).toString(),
                                                          m.value( "track" ).toString(),
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
        rp->setPurchaseUrl( m.value( "purchaseUrl" ).toString() );
        rp->setLinkUrl( m.value( "linkUrl" ).toString() );
        rp->setScore( m.value( "score" ).toFloat() );
        rp->setChecked( m.value( "checked" ).toBool() );

        //FIXME
        if ( m.contains( "year" ) )
        {
            QVariantMap attr;
            attr[ "releaseyear" ] = m.value( "year" );
//            rp->track()->setAttributes( attr );
        }

        rp->setMimetype( m.value( "mimetype" ).toString() );
        if ( rp->mimetype().isEmpty() )
        {
            rp->setMimetype( TomahawkUtils::extensionToMimetype( m.value( "extension" ).toString() ) );
            Q_ASSERT( !rp->mimetype().isEmpty() );
        }

        rp->setFriendlySource( name() );

        // find collection
        const QString collectionId = m.value( "collectionId" ).toString();
        if ( !collectionId.isEmpty() )
        {
            if ( scriptCollection( collectionId ).isNull() )
            {
                tLog() << "Resolver returned invalid collection id";
                Q_ASSERT( false );
            }
            else
            {
                rp->setResolvedByCollection( scriptCollection( collectionId ) );
            }
        }

        results << rp;
    }

    return results;
}


QSharedPointer< ScriptCollection >
ScriptAccount::scriptCollection( const QString& id ) const
{
    return m_collectionFactory->scriptPlugins().value( id );
}
