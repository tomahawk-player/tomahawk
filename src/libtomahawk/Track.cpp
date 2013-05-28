/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "Track.h"

#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "database/DatabaseCommand_LogPlayback.h"
#include "database/DatabaseCommand_ModifyInboxEntry.h"
#include "Album.h"
#include "collection/Collection.h"
#include "Pipeline.h"
#include "resolvers/Resolver.h"
#include "SourceList.h"
#include "audio/AudioEngine.h"
#include "TrackData.h"
#include "SocialAction.h"
#include "infosystem/InfoSystem.h"

#include "utils/Logger.h"

#include <QPixmap>
#include <QtAlgorithms>
#include <QReadWriteLock>

using namespace Tomahawk;

QHash< QString, track_wptr > Track::s_tracksByName = QHash< QString, track_wptr >();

static QMutex s_nameCacheMutex;


inline QString
cacheKey( const QString& artist, const QString& track, const QString& album, int duration, const QString& composer, unsigned int albumpos, unsigned int discnumber )
{
    QString str;
    QTextStream stream( &str );
    stream << artist << track << album << composer << duration << albumpos << discnumber;
    return str;
}


track_ptr
Track::get( const QString& artist, const QString& track, const QString& album, int duration, const QString& composer, unsigned int albumpos, unsigned int discnumber )
{
    if ( artist.trimmed().isEmpty() || track.trimmed().isEmpty() )
    {
        Q_ASSERT( false );
        return track_ptr();
    }

    QMutexLocker lock( &s_nameCacheMutex );
    const QString key = cacheKey( artist, track, album, duration, composer, albumpos, discnumber );
    if ( s_tracksByName.contains( key ) )
    {
        track_wptr track = s_tracksByName.value( key );
        if ( track )
            return track.toStrongRef();
    }

    track_ptr t = track_ptr( new Track( artist, track, album, duration, composer, albumpos, discnumber ), &Track::deleteLater );
    t->setWeakRef( t.toWeakRef() );
    s_tracksByName.insert( key, t );

    return t;
}


track_ptr
Track::get( unsigned int id, const QString& artist, const QString& track, const QString& album, int duration, const QString& composer, unsigned int albumpos, unsigned int discnumber )
{
    QMutexLocker lock( &s_nameCacheMutex );
    const QString key = cacheKey( artist, track, album, duration, composer, albumpos, discnumber );
    if ( s_tracksByName.contains( key ) )
    {
        track_wptr track = s_tracksByName.value( key );
        if ( track )
            return track;
    }

    track_ptr t = track_ptr( new Track( id, artist, track, album, duration, composer, albumpos, discnumber ), &Track::deleteLater );
    t->setWeakRef( t.toWeakRef() );
    s_tracksByName.insert( key, t );

    return t;
}


Track::Track( unsigned int id, const QString& artist, const QString& track, const QString& album, int duration, const QString& composer, unsigned int albumpos, unsigned int discnumber )
    : m_composer( composer )
    , m_album( album )
    , m_duration( duration )
    , m_albumpos( albumpos )
    , m_discnumber( discnumber )
{
    m_trackData = TrackData::get( id, artist, track );

    init();
}


Track::Track( const QString& artist, const QString& track, const QString& album, int duration, const QString& composer, unsigned int albumpos, unsigned int discnumber )
    : m_composer( composer )
    , m_album( album )
    , m_duration( duration )
    , m_albumpos( albumpos )
    , m_discnumber( discnumber )
{
    m_trackData = TrackData::get( 0, artist, track );

    init();
}


Track::~Track()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << toString();
}


void
Track::init()
{
    updateSortNames();

    connect( m_trackData.data(), SIGNAL( attributesLoaded() ), SIGNAL( attributesLoaded() ) );
    connect( m_trackData.data(), SIGNAL( socialActionsLoaded() ), SIGNAL( socialActionsLoaded() ) );
    connect( m_trackData.data(), SIGNAL( statsLoaded() ), SIGNAL( statsLoaded() ) );
    connect( m_trackData.data(), SIGNAL( similarTracksLoaded() ), SIGNAL( similarTracksLoaded() ) );
    connect( m_trackData.data(), SIGNAL( lyricsLoaded() ), SIGNAL( lyricsLoaded() ) );
}


void
Track::deleteLater()
{
    QMutexLocker lock( &s_nameCacheMutex );

    const QString key = cacheKey( artist(), track(), m_album, m_duration, m_composer, m_albumpos, m_discnumber );
    if ( s_tracksByName.contains( key ) )
    {
        s_tracksByName.remove( key );
    }

    QObject::deleteLater();
}


unsigned int
Track::trackId() const
{
    return m_trackData->trackId();
}


void
Track::startPlaying()
{
    DatabaseCommand_LogPlayback* cmd = new DatabaseCommand_LogPlayback( weakRef().toStrongRef(),
                                                                        DatabaseCommand_LogPlayback::Started );
    Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );


}


void
Track::finishPlaying( int timeElapsed )
{
    DatabaseCommand_LogPlayback* cmd = new DatabaseCommand_LogPlayback( weakRef().toStrongRef(),
                                                                        DatabaseCommand_LogPlayback::Finished,
                                                                        timeElapsed );
    Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );

    bool isUnlistened = false;
    foreach ( Tomahawk::SocialAction action, allSocialActions() )
    {
        if ( action.action == "Inbox" && action.value.toBool() == true )
        {
            isUnlistened = true;
            break;
        }
    }

    if ( isUnlistened )
    {
        DatabaseCommand_ModifyInboxEntry* cmd = new DatabaseCommand_ModifyInboxEntry( toQuery(), false );
        Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );

        // The dbcmd does this in the DB, but let's update the TrackData ASAP
        QList< Tomahawk::SocialAction > actions = allSocialActions();
        for ( QList< Tomahawk::SocialAction >::iterator it = actions.begin();
              it != actions.end(); ++it )
        {
            if ( it->action == "Inbox" )
            {
                it->value = false; //listened!
            }
        }
        m_trackData->setAllSocialActions( actions ); //emits socialActionsLoaded which gets propagated here
    }
}


void
Track::updateSortNames()
{
    m_composerSortname = DatabaseImpl::sortname( m_composer, true );
    m_albumSortname = DatabaseImpl::sortname( m_album );
}


bool
Track::equals( const Tomahawk::track_ptr& other, bool ignoreCase ) const
{
    if ( other.isNull() )
        return false;

    if ( ignoreCase )
        return ( artist().toLower() == other->artist().toLower() &&
                 album().toLower() == other->album().toLower() &&
                 track().toLower() == other->track().toLower() );
    else
        return ( artist() == other->artist() &&
                 album() == other->album() &&
                 track() == other->track() );
}


QVariant
Track::toVariant() const
{
    QVariantMap m;
    m.insert( "artist", artist() );
    m.insert( "album", album() );
    m.insert( "track", track() );
    m.insert( "duration", duration() );

    return m;
}


QString
Track::toString() const
{
    return QString( "Track(%1 - %2%3)" )
              .arg( artist() )
              .arg( track() )
              .arg( album().isEmpty() ? "" : QString( " on %1" ).arg( album() ) );
}


query_ptr
Track::toQuery()
{
    if ( m_query.isNull() )
    {
        query_ptr query = Tomahawk::Query::get( weakRef().toStrongRef() );
        if ( !query )
            return query_ptr();

        m_query = query->weakRef();
        return query;
    }

    return m_query.toStrongRef();
}


void
Track::loadStats()
{
    m_trackData->loadStats();
}


QList< Tomahawk::PlaybackLog >
Track::playbackHistory( const Tomahawk::source_ptr& source ) const
{
    return m_trackData->playbackHistory( source );
}


unsigned int
Track::playbackCount( const source_ptr& source )
{
    return m_trackData->playbackCount( source );
}


void
Track::loadAttributes()
{
    m_trackData->loadAttributes();
}


void
Track::loadSocialActions()
{
    m_trackData->loadSocialActions();
}


QList< SocialAction >
Track::allSocialActions() const
{
    return m_trackData->allSocialActions();
}


bool
Track::loved()
{
    return m_trackData->loved();
}


void
Track::setLoved( bool loved, bool postToInfoSystem )
{
    m_trackData->setLoved( loved );

    if ( postToInfoSystem )
    {
        QVariantMap loveInfo;
        Tomahawk::InfoSystem::InfoStringHash trackInfo;
        trackInfo["title"] = track();
        trackInfo["artist"] = artist();
        trackInfo["album"] = album();

        loveInfo[ "trackinfo" ] = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );

        Tomahawk::InfoSystem::InfoPushData pushData ( m_trackData->id(),
                                                    ( loved ? Tomahawk::InfoSystem::InfoLove : Tomahawk::InfoSystem::InfoUnLove ),
                                                    loveInfo,
                                                    Tomahawk::InfoSystem::PushShortUrlFlag );

        Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( pushData );
    }
}


QString
Track::socialActionDescription( const QString& action, DescriptionMode mode ) const
{
    QString desc;
    QList< Tomahawk::SocialAction > socialActions = allSocialActions();

    QStringList actionSources;
    int loveTotal = 0;
    foreach ( const Tomahawk::SocialAction& sa, socialActions )
    {
        if ( sa.action == action )
        {
            if ( actionSources.contains( sa.source->friendlyName() ) )
                continue;
            actionSources << sa.source->friendlyName();
            loveTotal++;
        }
    }

    QDateTime earliestTimestamp = QDateTime::currentDateTime();
    actionSources.clear();
    int loveCounter = 0;
    foreach ( const Tomahawk::SocialAction& sa, socialActions )
    {
        if ( sa.action == action )
        {
            if ( actionSources.contains( sa.source->friendlyName() ) )
                continue;
            actionSources << sa.source->friendlyName();

            if ( ++loveCounter > 3 )
                continue;
            else if ( loveCounter > 1 )
            {
                if ( loveCounter == loveTotal )
                    desc += tr( " and " );
                else
                    desc += ", ";
            }

            if ( sa.source->isLocal() )
            {
                if ( loveCounter == 1 )
                    desc += "<b>" + tr( "You" ) + "</b>";
                else
                    desc += "<b>" + tr( "you" ) + "</b>";
            }
            else
                desc += "<b>" + sa.source->friendlyName() + "</b>";

            QDateTime saTimestamp = QDateTime::fromTime_t( sa.timestamp.toInt() );
            if ( saTimestamp < earliestTimestamp && saTimestamp.toTime_t() > 0 )
                earliestTimestamp = saTimestamp;
        }
    }
    if ( loveCounter > 0 )
    {
        if ( loveCounter > 3 )
            desc += " " + tr( "and" ) + " <b>" + tr( "%n other(s)", "", loveCounter - 3 ) + "</b>";

        if ( mode == Short )
            desc = "<b>" + tr( "%n people", "", loveCounter ) + "</b>";

         //FIXME: more action descs required
        if ( action == "Love" )
            desc += " " + tr( "loved this track" );
        else if ( action == "Inbox" )
            desc += " " + tr( "sent you this track %1" )
                    .arg( TomahawkUtils::ageToString( earliestTimestamp, true ) );
    }

    return desc;
}


artist_ptr
Track::artistPtr() const
{
    if ( !m_artistPtr )
    {
        m_artistPtr = Artist::get( artist(), false );
        connect( m_artistPtr.data(), SIGNAL( updated() ), SIGNAL( updated() ), Qt::UniqueConnection );
        connect( m_artistPtr.data(), SIGNAL( coverChanged() ), SIGNAL( coverChanged() ), Qt::UniqueConnection );
    }

    return m_artistPtr;
}


album_ptr
Track::albumPtr() const
{
    if ( !m_albumPtr )
    {
        m_albumPtr = Album::get( artistPtr(), album(), false );
        connect( m_albumPtr.data(), SIGNAL( updated() ), SIGNAL( updated() ), Qt::UniqueConnection );
        connect( m_albumPtr.data(), SIGNAL( coverChanged() ), SIGNAL( coverChanged() ), Qt::UniqueConnection );
    }

    return m_albumPtr;
}


artist_ptr
Track::composerPtr() const
{
    if ( !m_composerPtr )
        m_composerPtr = Artist::get( composer(), false );

    return m_composerPtr;
}


#ifndef ENABLE_HEADLESS
QPixmap
Track::cover( const QSize& size, bool forceLoad ) const
{
    albumPtr()->cover( size, forceLoad );
    if ( albumPtr()->coverLoaded() )
    {
        if ( !albumPtr()->cover( size ).isNull() )
            return albumPtr()->cover( size );

        return artistPtr()->cover( size, forceLoad );
    }

    return QPixmap();
}


bool
Track::coverLoaded() const
{
    if ( m_albumPtr.isNull() )
        return false;

    if ( m_albumPtr->coverLoaded() && !m_albumPtr->cover( QSize( 0, 0 ) ).isNull() )
        return true;

    return m_artistPtr->coverLoaded();
}

#endif


QList<Tomahawk::query_ptr>
Track::similarTracks() const
{
    return m_trackData->similarTracks();
}


QStringList
Track::lyrics() const
{
    return m_trackData->lyrics();
}


QString
Track::artist() const
{
    return m_trackData->artist();
}


QString
Track::track() const
{
    return m_trackData->track();
}


QVariantMap
Track::attributes() const
{
    return m_trackData->attributes();
}


int
Track::year() const
{
    return m_trackData->year();
}


void
Track::share( const Tomahawk::source_ptr& source )
{
    m_trackData->share( source );
}


QString
Track::artistSortname() const
{
    return m_trackData->artistSortname();
}


QString
Track::trackSortname() const
{
    return m_trackData->trackSortname();
}
