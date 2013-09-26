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

#include "TrackData.h"

#include <QtAlgorithms>
#include <QReadWriteLock>

#include "audio/AudioEngine.h"
#include "collection/Collection.h"
#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "database/DatabaseCommand_LogPlayback.h"
#include "database/DatabaseCommand_LoadPlaylistEntries.h"
#include "database/DatabaseCommand_LoadSocialActions.h"
#include "database/DatabaseCommand_LoadTrackAttributes.h"
#include "database/DatabaseCommand_ShareTrack.h"
#include "database/DatabaseCommand_SocialAction.h"
#include "database/DatabaseCommand_TrackStats.h"
#include "database/IdThreadWorker.h"
#include "resolvers/Resolver.h"
#include "utils/Logger.h"

#include "Album.h"
#include "PlaylistEntry.h"
#include "SourceList.h"

using namespace Tomahawk;

QHash< QString, trackdata_wptr > TrackData::s_trackDatasByName = QHash< QString, trackdata_wptr >();
QHash< unsigned int, trackdata_wptr > TrackData::s_trackDatasById = QHash< unsigned int, trackdata_wptr >();

static QMutex s_datanameCacheMutex;
static QMutex s_memberMutex;
static QReadWriteLock s_dataidMutex;

inline QString
cacheKey( const QString& artist, const QString& track )
{
    QString str;
    QTextStream stream( &str );
    stream << DatabaseImpl::sortname( artist ) << "\t" << DatabaseImpl::sortname( track );
    return str;
}


trackdata_ptr
TrackData::get( unsigned int id, const QString& artist, const QString& track )
{
    s_dataidMutex.lockForRead();
    if ( s_trackDatasById.contains( id ) )
    {
        trackdata_wptr track = s_trackDatasById.value( id );
        s_dataidMutex.unlock();

        if ( track )
            return track;
    }
    s_dataidMutex.unlock();

    QMutexLocker lock( &s_datanameCacheMutex );
    const QString key = cacheKey( artist, track );
    if ( s_trackDatasByName.contains( key ) )
    {
        trackdata_wptr track = s_trackDatasByName.value( key );
        if ( track )
            return track;
    }

    trackdata_ptr t = trackdata_ptr( new TrackData( id, artist, track ), &TrackData::deleteLater );
    t->setWeakRef( t.toWeakRef() );
    s_trackDatasByName.insert( key, t );

    if ( id > 0 )
    {
        s_dataidMutex.lockForWrite();
        s_trackDatasById.insert( id, t );
        s_dataidMutex.unlock();
    }
    else
        t->loadId( false );

    return t;
}


TrackData::TrackData( unsigned int id, const QString& artist, const QString& track )
    : m_artist( artist )
    , m_track( track )
    , m_year( 0 )
    , m_attributesLoaded( false )
    , m_socialActionsLoaded( false )
    , m_playbackHistoryLoaded( false )
    , m_chartPosition( 0 )
    , m_chartCount( 0 )
    , m_simTracksLoaded( false )
    , m_lyricsLoaded( false )
    , m_infoJobs( 0 )
    , m_trackId( id )
{
    m_waitingForId = ( id == 0 );

    updateSortNames();
}


TrackData::~TrackData()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << m_artist << m_track;
}


void
TrackData::deleteLater()
{
    QMutexLocker lock( &s_datanameCacheMutex );

    const QString key = cacheKey( m_artist, m_track );
    if ( s_trackDatasByName.contains( key ) )
    {
        s_trackDatasByName.remove( key );
    }

    if ( m_trackId > 0 )
    {
        s_dataidMutex.lockForWrite();
        if ( s_trackDatasById.contains( m_trackId ) )
        {
            s_trackDatasById.remove( m_trackId );
        }
        s_dataidMutex.unlock();
    }

    QObject::deleteLater();
}


void
TrackData::updateSortNames()
{
    m_artistSortname = DatabaseImpl::sortname( m_artist, true );
    m_trackSortname = DatabaseImpl::sortname( m_track );
}


QString
TrackData::toString() const
{
    return QString( "TrackData(%1, %2 - %3)" )
              .arg( m_trackId )
              .arg( m_artist )
              .arg( m_track );
}


query_ptr
TrackData::toQuery()
{
    return Tomahawk::Query::get( m_artist, m_track, "" );
}


void
TrackData::loadId( bool autoCreate ) const
{
    Q_ASSERT( m_waitingForId );
    IdThreadWorker::getTrackId( m_ownRef.toStrongRef(), autoCreate );
}


void
TrackData::setIdFuture( QFuture<unsigned int> future )
{
    m_idFuture = future;
}


unsigned int
TrackData::trackId() const
{
    s_dataidMutex.lockForRead();
    const bool waiting = m_waitingForId;
    unsigned int finalId = m_trackId;
    s_dataidMutex.unlock();

    if ( waiting )
    {
        finalId = m_idFuture.result();

        s_dataidMutex.lockForWrite();
        m_trackId = finalId;
        m_waitingForId = false;

        if ( m_trackId > 0 )
        {
            s_trackDatasById.insert( m_trackId, m_ownRef.toStrongRef() );
        }

        s_dataidMutex.unlock();
    }

    return finalId;
}


void
TrackData::loadAttributes()
{
    if ( m_attributesLoaded )
        return;

    m_attributesLoaded = true;

    DatabaseCommand_LoadTrackAttributes* cmd = new DatabaseCommand_LoadTrackAttributes( m_ownRef.toStrongRef() );
    Database::instance()->enqueue( Tomahawk::dbcmd_ptr(cmd) );
}


void
TrackData::updateAttributes()
{
    if ( m_attributes.contains( "releaseyear" ) )
    {
        m_year = m_attributes.value( "releaseyear" ).toInt();
    }

    emit attributesLoaded();
}


void
TrackData::loadSocialActions( bool force )
{
    if ( !force && m_socialActionsLoaded )
        return;

    m_socialActionsLoaded = true;

    DatabaseCommand_LoadSocialActions* cmd = new DatabaseCommand_LoadSocialActions( m_ownRef.toStrongRef() );
    Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
}


void
TrackData::setAllSocialActions( const QList< SocialAction >& socialActions )
{
    {
        QMutexLocker locker( &s_memberMutex );
        m_allSocialActions = socialActions;
        parseSocialActions();
    }

    emit socialActionsLoaded();
}


QList< SocialAction >
TrackData::allSocialActions() const
{
    QMutexLocker locker( &s_memberMutex );
    return m_allSocialActions;
}


QList< Tomahawk::SocialAction >
TrackData::socialActions( const QString& actionName, const QVariant& value, bool filterDupeSourceNames )
{
    QMutexLocker locker( &s_memberMutex );

    QList< Tomahawk::SocialAction > filtered;
    foreach ( const Tomahawk::SocialAction& sa, m_allSocialActions )
    {
        if ( sa.action == actionName )
        {
            if ( !value.isNull() && sa.value != value )
            {
                Tomahawk::source_ptr srcWithBadValue = sa.source;
                QList< Tomahawk::SocialAction>::iterator it = filtered.begin();
                while ( it != filtered.end() )
                {
                    if ( it->source == srcWithBadValue )
                        it = filtered.erase( it );
                    else
                        ++it;
                }
                continue;
            }

            bool dupe = false;
            for ( QList< Tomahawk::SocialAction>::iterator it = filtered.begin();
                  it != filtered.end(); ++it )
            {
                if ( it->source == sa.source ||
                        ( filterDupeSourceNames &&
                            ( it->source->friendlyName() == sa.source->friendlyName() ) ) )
                {
                    dupe = true;
                    // we store the earliest timestamp in the sa we're keeping
                    if ( it->timestamp.toInt() > sa.timestamp.toInt() )
                        it->timestamp = sa.timestamp;
                    // and always the new value
                    it->value = sa.value;
                    break;
                }
            }

            if ( dupe )
                continue;
            filtered << sa;
       }
    }

    return filtered;
}


void
TrackData::parseSocialActions()
{
    QListIterator< Tomahawk::SocialAction > it( m_allSocialActions );
    unsigned int highestTimestamp = 0;

    while ( it.hasNext() )
    {
        Tomahawk::SocialAction socialAction;
        socialAction = it.next();
        if ( socialAction.timestamp.toUInt() > highestTimestamp && socialAction.source->isLocal() )
        {
            m_currentSocialActions[ socialAction.action.toString() ] = socialAction.value.toBool();
        }
    }
}


bool
TrackData::loved()
{
    QMutexLocker locker( &s_memberMutex );

    if ( m_socialActionsLoaded )
    {
        return m_currentSocialActions[ "Love" ].toBool();
    }
    else
    {
        loadSocialActions();
    }

    return false;
}


void
TrackData::setLoved( bool loved )
{
    m_currentSocialActions[ "Love" ] = loved;
    emit socialActionsLoaded();

    DatabaseCommand_SocialAction* cmd = new DatabaseCommand_SocialAction( m_ownRef.toStrongRef(), QString( "Love" ), loved ? QString( "true" ) : QString( "false" ) );
    Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
}


void
TrackData::share( const Tomahawk::source_ptr& source )
{
    DatabaseCommand_ShareTrack* cmd = new DatabaseCommand_ShareTrack( m_ownRef.toStrongRef(), source->nodeId() );
    Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
}


void
TrackData::loadStats()
{
    if ( m_playbackHistoryLoaded )
        return;

    m_playbackHistoryLoaded = true;

    DatabaseCommand_TrackStats* cmd = new DatabaseCommand_TrackStats( m_ownRef.toStrongRef() );
    connect( cmd, SIGNAL( trackStats( unsigned int, unsigned int ) ), SLOT( onTrackStatsLoaded( unsigned int, unsigned int ) ) );
    Database::instance()->enqueue( Tomahawk::dbcmd_ptr(cmd) );
}


QList< Tomahawk::PlaybackLog >
TrackData::playbackHistory( const Tomahawk::source_ptr& source ) const
{
    QMutexLocker locker( &s_memberMutex );

    QList< Tomahawk::PlaybackLog > history;
    foreach ( const PlaybackLog& log, m_playbackHistory )
    {
        if ( source.isNull() || log.source == source )
        {
            history << log;
        }
    }

    return history;
}


void
TrackData::setPlaybackHistory( const QList< Tomahawk::PlaybackLog >& playbackData )
{
    {
        QMutexLocker locker( &s_memberMutex );
        m_playbackHistory = playbackData;
    }
    emit statsLoaded();
}


unsigned int
TrackData::playbackCount( const source_ptr& source )
{
    QMutexLocker locker( &s_memberMutex );

    unsigned int count = 0;
    foreach ( const PlaybackLog& log, m_playbackHistory )
    {
        if ( source.isNull() || log.source == source )
            count++;
    }

    return count;
}


void
TrackData::onTrackStatsLoaded( unsigned int chartPos, unsigned int chartCount )
{
    m_chartPosition = chartPos;
    m_chartCount = chartCount;

    emit statsLoaded();
}


unsigned int
TrackData::chartPosition() const
{
    return m_chartPosition;
}


unsigned int
TrackData::chartCount() const
{
    return m_chartCount;
}


QList<Tomahawk::query_ptr>
TrackData::similarTracks() const
{
    if ( !m_simTracksLoaded )
    {
        Tomahawk::InfoSystem::InfoStringHash trackInfo;
        trackInfo["artist"] = artist();
        trackInfo["track"] = track();

        Tomahawk::InfoSystem::InfoRequestData requestData;
        requestData.caller = id();
        requestData.customData = QVariantMap();

        requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );
        requestData.type = Tomahawk::InfoSystem::InfoTrackSimilars;
        requestData.requestId = TomahawkUtils::infosystemRequestId();

        connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                 SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                 SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ), Qt::UniqueConnection );

        connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                 SIGNAL( finished( QString ) ),
                 SLOT( infoSystemFinished( QString ) ), Qt::UniqueConnection );

        m_infoJobs++;
        Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );
    }

    return m_similarTracks;
}


QStringList
TrackData::lyrics() const
{
    if ( !m_lyricsLoaded )
    {
        Tomahawk::InfoSystem::InfoStringHash trackInfo;
        trackInfo["artist"] = artist();
        trackInfo["track"] = track();

        Tomahawk::InfoSystem::InfoRequestData requestData;
        requestData.caller = id();
        requestData.customData = QVariantMap();

        requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );
        requestData.type = Tomahawk::InfoSystem::InfoTrackLyrics;
        requestData.requestId = TomahawkUtils::infosystemRequestId();

        connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                 SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                 SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ), Qt::UniqueConnection );

        connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                 SIGNAL( finished( QString ) ),
                 SLOT( infoSystemFinished( QString ) ), Qt::UniqueConnection );

        m_infoJobs++;
        Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );
    }

    return m_lyrics;
}


void
TrackData::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != id() )
        return;

    QVariantMap returnedData = output.value< QVariantMap >();
    switch ( requestData.type )
    {
        case InfoSystem::InfoTrackLyrics:
        {
            m_lyrics = output.value< QVariant >().toString().split( "\n" );

            m_lyricsLoaded = true;
            emit lyricsLoaded();
            break;
        }

        case InfoSystem::InfoTrackSimilars:
        {
            const QStringList artists = returnedData["artists"].toStringList();
            const QStringList tracks = returnedData["tracks"].toStringList();

            for ( int i = 0; i < tracks.count() && i < 50; i++ )
            {
                m_similarTracks << Query::get( artists.at( i ), tracks.at( i ), QString(), uuid(), false );
            }

            m_simTracksLoaded = true;
            emit similarTracksLoaded();

            break;
        }

        default:
            Q_ASSERT( false );
    }
}


void
TrackData::infoSystemFinished( QString target )
{
    if ( target != id() )
        return;

    if ( --m_infoJobs == 0 )
    {
        disconnect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                    this, SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

        disconnect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ),
                    this, SLOT( infoSystemFinished( QString ) ) );
    }
}


QString
TrackData::id() const
{
    if ( m_uuid.isEmpty() )
    {
        m_uuid = uuid();
    }

    return m_uuid;
}
