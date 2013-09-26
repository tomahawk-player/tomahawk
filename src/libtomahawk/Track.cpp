/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#include "Track_p.h"

#include "audio/AudioEngine.h"
#include "collection/Collection.h"
#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "database/DatabaseCommand_LogPlayback.h"
#include "database/DatabaseCommand_ModifyInboxEntry.h"
#include "resolvers/Resolver.h"
#include "utils/Logger.h"

#include "Album.h"
#include "Pipeline.h"
#include "PlaylistEntry.h"
#include "SourceList.h"

#include <QtAlgorithms>
#include <QDateTime>
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
    : d_ptr( new TrackPrivate( this, album, duration, composer, albumpos, discnumber ) )
{
    Q_D( Track );
    d->trackData = TrackData::get( id, artist, track );

    init();
}


Track::Track( const QString& artist, const QString& track, const QString& album, int duration, const QString& composer, unsigned int albumpos, unsigned int discnumber )
    : d_ptr( new TrackPrivate( this, album, duration, composer, albumpos, discnumber ) )
{
    Q_D( Track );
    d->trackData = TrackData::get( 0, artist, track );

    init();
}


Track::~Track()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << toString();
}


void
Track::setArtist( const QString& artist )
{
    Q_D( Track );

    d->artistPtr = artist_ptr();
    d->trackData = TrackData::get( 0, artist, track() );

    init();
    emit updated();
}


void
Track::setAlbum( const QString& album )
{
    Q_D( Track );

    d->albumPtr = album_ptr();
    d->album = album;
    updateSortNames();

    emit updated();
}


void
Track::setTrack( const QString& track )
{
    Q_D( Track );

    d->trackData = TrackData::get( 0, artist(), track );

    init();
    emit updated();
}


void
Track::setAlbumPos( unsigned int albumpos )
{
    Q_D( Track );

    d->albumpos = albumpos;

    emit updated();
}


void
Track::setAttributes( const QVariantMap& map )
{
    Q_D( Track );

    d->trackData->setAttributes( map );

    emit attributesLoaded();
}


void
Track::init()
{
    Q_D( Track );
    updateSortNames();

    connect( d->trackData.data(), SIGNAL( attributesLoaded() ), SIGNAL( attributesLoaded() ) );
    connect( d->trackData.data(), SIGNAL( socialActionsLoaded() ), SIGNAL( socialActionsLoaded() ) );
    connect( d->trackData.data(), SIGNAL( statsLoaded() ), SIGNAL( statsLoaded() ) );
    connect( d->trackData.data(), SIGNAL( similarTracksLoaded() ), SIGNAL( similarTracksLoaded() ) );
    connect( d->trackData.data(), SIGNAL( lyricsLoaded() ), SIGNAL( lyricsLoaded() ) );
}


void
Track::deleteLater()
{
    Q_D( Track );
    QMutexLocker lock( &s_nameCacheMutex );

    const QString key = cacheKey( artist(), track(), d->album, d->duration, d->composer, d->albumpos, d->discnumber );
    if ( s_tracksByName.contains( key ) )
    {
        s_tracksByName.remove( key );
    }

    QObject::deleteLater();
}


unsigned int
Track::trackId() const
{
    Q_D( const Track );
    return d->trackData->trackId();
}


QWeakPointer<Track>
Track::weakRef()
{
    Q_D( Track );
    return d->ownRef;
}


void
Track::setWeakRef( QWeakPointer<Track> weakRef )
{
    Q_D( Track );
    d->ownRef = weakRef;
}


void
Track::startPlaying()
{
    DatabaseCommand_LogPlayback* cmd = new DatabaseCommand_LogPlayback( weakRef().toStrongRef(),
                                                                        DatabaseCommand_LogPlayback::Started );
    Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );

    markAsListened();
}


void
Track::finishPlaying( int timeElapsed )
{
    DatabaseCommand_LogPlayback* cmd = new DatabaseCommand_LogPlayback( weakRef().toStrongRef(),
                                                                        DatabaseCommand_LogPlayback::Finished,
                                                                        timeElapsed );
    Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
}


void
Track::markAsListened()
{
    Q_D( Track );
    if ( !isListened() )
    {
        DatabaseCommand_ModifyInboxEntry* cmd = new DatabaseCommand_ModifyInboxEntry( toQuery(), false );
        Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );

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
        d->trackData->blockSignals( true );
        d->trackData->setAllSocialActions( actions ); //emits socialActionsLoaded which gets propagated here
        d->trackData->blockSignals( false );
    }
}


bool
Track::isListened() const
{
    bool isUnlistened = false;
    foreach ( Tomahawk::SocialAction action, allSocialActions() )
    {
        if ( action.action == "Inbox" && action.value.toBool() == true )
        {
            isUnlistened = true;
            break;
        }
    }
    return !isUnlistened;
}


void
Track::updateSortNames()
{
    Q_D( Track );
    d->composerSortname = DatabaseImpl::sortname( d->composer, true );
    d->albumSortname = DatabaseImpl::sortname( d->album );
}


void
Track::setAllSocialActions( const QList< SocialAction >& socialActions )
{
    Q_D( Track );
    d->trackData->setAllSocialActions( socialActions );
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
    Q_D( Track );
    if ( d->query.isNull() )
    {
        query_ptr query = Tomahawk::Query::get( weakRef().toStrongRef() );
        if ( !query )
            return query_ptr();

        d->query = query->weakRef();
        return query;
    }

    return d->query.toStrongRef();
}


QString
Track::composerSortname() const
{
    Q_D( const Track );
    return d->composerSortname;
}


QString
Track::albumSortname() const
{
    Q_D( const Track );
    return d->albumSortname;
}


void
Track::loadStats()
{
    Q_D( Track );
    d->trackData->loadStats();
}


QList< Tomahawk::PlaybackLog >
Track::playbackHistory( const Tomahawk::source_ptr& source ) const
{
    Q_D( const Track );
    return d->trackData->playbackHistory( source );
}


unsigned int
Track::playbackCount( const source_ptr& source )
{
    Q_D( Track );
    return d->trackData->playbackCount( source );
}


unsigned int
Track::chartPosition() const
{
    Q_D( const Track );
    return d->trackData->chartPosition();
}


unsigned int
Track::chartCount() const
{
    Q_D( const Track );
    return d->trackData->chartCount();
}


void
Track::loadAttributes()
{
    Q_D( Track );
    d->trackData->loadAttributes();
}


void
Track::loadSocialActions( bool force )
{
    Q_D( Track );

    d->trackData->loadSocialActions( force );
}


QList< SocialAction >
Track::allSocialActions() const
{
    Q_D( const Track );
    return d->trackData->allSocialActions();
}


bool
Track::loved()
{
    Q_D( Track );
    return d->trackData->loved();
}


void
Track::setLoved( bool loved, bool postToInfoSystem )
{
    Q_D( Track );
    d->trackData->setLoved( loved );

    if ( postToInfoSystem )
    {
        QVariantMap loveInfo;
        Tomahawk::InfoSystem::InfoStringHash trackInfo;
        trackInfo["title"] = track();
        trackInfo["artist"] = artist();
        trackInfo["album"] = album();

        loveInfo[ "trackinfo" ] = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );

        Tomahawk::InfoSystem::InfoPushData pushData ( d->trackData->id(),
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


QList< Tomahawk::SocialAction >
Track::socialActions( const QString& actionName, const QVariant& value, bool filterDupeSourceNames )
{
    Q_D( Track );
    return d->trackData->socialActions( actionName, value, filterDupeSourceNames );
}


artist_ptr
Track::artistPtr() const
{
    Q_D( const Track );
    if ( !d->artistPtr )
    {
        d->artistPtr = Artist::get( artist(), false );
        connect( d->artistPtr.data(), SIGNAL( updated() ), SIGNAL( updated() ), Qt::UniqueConnection );
        connect( d->artistPtr.data(), SIGNAL( coverChanged() ), SIGNAL( coverChanged() ), Qt::UniqueConnection );
    }

    return d->artistPtr;
}


album_ptr
Track::albumPtr() const
{
    Q_D( const Track );
    if ( !d->albumPtr )
    {
        d->albumPtr = Album::get( artistPtr(), album(), false );
        connect( d->albumPtr.data(), SIGNAL( updated() ), SIGNAL( updated() ), Qt::UniqueConnection );
        connect( d->albumPtr.data(), SIGNAL( coverChanged() ), SIGNAL( coverChanged() ), Qt::UniqueConnection );
    }

    return d->albumPtr;
}


artist_ptr
Track::composerPtr() const
{
    Q_D( const Track );
    if ( !d->composerPtr )
        d->composerPtr = Artist::get( composer(), false );

    return d->composerPtr;
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
    Q_D( const Track );
    if ( d->albumPtr.isNull() )
        return false;

    if ( d->albumPtr->coverLoaded() && !d->albumPtr->cover( QSize( 0, 0 ) ).isNull() )
        return true;

    return d->artistPtr->coverLoaded();
}

#endif


QList<Tomahawk::query_ptr>
Track::similarTracks() const
{
    Q_D( const Track );
    return d->trackData->similarTracks();
}


QStringList
Track::lyrics() const
{
    Q_D( const Track );
    return d->trackData->lyrics();
}


QString
Track::artist() const
{
    Q_D( const Track );
    return d->trackData->artist();
}


QString
Track::track() const
{
    Q_D( const Track );
    return d->trackData->track();
}


QString
Track::composer() const
{
    Q_D( const Track );
    return d->composer;
}


QString
Track::album() const
{
    Q_D( const Track );
    return d->album;
}


int
Track::duration() const
{
    Q_D( const Track );
    return d->duration;
}


QVariantMap
Track::attributes() const
{
    Q_D( const Track );
    return d->trackData->attributes();
}


int
Track::year() const
{
    Q_D( const Track );
    return d->trackData->year();
}


unsigned int
Track::albumpos() const
{
    Q_D( const Track );
    return d->albumpos;
}


unsigned int
Track::discnumber() const
{
    Q_D( const Track );
    return d->discnumber;
}


void
Track::share( const Tomahawk::source_ptr& source )
{
    Q_D( Track );
    d->trackData->share( source );
}


QString
Track::artistSortname() const
{
    Q_D( const Track );
    return d->trackData->artistSortname();
}


QString
Track::trackSortname() const
{
    Q_D( const Track );
    return d->trackData->trackSortname();
}
