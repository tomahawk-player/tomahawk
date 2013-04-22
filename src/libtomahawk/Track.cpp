/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include <QtAlgorithms>

#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "database/DatabaseCommand_LogPlayback.h"
#include "database/DatabaseCommand_LoadPlaylistEntries.h"
#include "database/DatabaseCommand_LoadSocialActions.h"
#include "database/DatabaseCommand_SocialAction.h"
#include "database/DatabaseCommand_TrackStats.h"
#include "Album.h"
#include "collection/Collection.h"
#include "Pipeline.h"
#include "resolvers/Resolver.h"
#include "SourceList.h"
#include "audio/AudioEngine.h"

#include "utils/Logger.h"

using namespace Tomahawk;

static QHash< QString, track_wptr > s_tracks;
static QMutex s_mutex;

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
        return track_ptr();

    QMutexLocker lock( &s_mutex );
    const QString key = cacheKey( artist, track, album, duration, composer, albumpos, discnumber );
    if ( s_tracks.contains( key ) )
    {
        return s_tracks.value( key );
    }

    track_ptr t = track_ptr( new Track( artist, track, album, duration, composer, albumpos, discnumber ), &Track::deleteLater );
    t->setWeakRef( t.toWeakRef() );
    s_tracks.insert( key, t );

    return t;
}


Track::Track( const QString& artist, const QString& track, const QString& album, int duration, const QString& composer, unsigned int albumpos, unsigned int discnumber )
    : m_artist( artist )
    , m_composer( composer )
    , m_album( album )
    , m_track( track )
    , m_duration( duration )
    , m_albumpos( albumpos )
    , m_discnumber( discnumber )
    , m_socialActionsLoaded( false )
    , m_simTracksLoaded( false )
    , m_lyricsLoaded( false )
    , m_infoJobs( 0 )
{
    updateSortNames();
}


Track::~Track()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << toString();
}


void
Track::deleteLater()
{
    QMutexLocker lock( &s_mutex );

    const QString key = cacheKey( m_artist, m_track, m_album, m_duration, m_composer, m_albumpos, m_discnumber );
    if ( s_tracks.contains( key ) )
    {
        s_tracks.remove( key );
    }

    QObject::deleteLater();
}


void
Track::updateSortNames()
{
    m_artistSortname = DatabaseImpl::sortname( m_artist, true );
    m_composerSortname = DatabaseImpl::sortname( m_composer, true );
    m_albumSortname = DatabaseImpl::sortname( m_album );
    m_trackSortname = DatabaseImpl::sortname( m_track );
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
    DatabaseCommand_TrackStats* cmd = new DatabaseCommand_TrackStats( m_ownRef.toStrongRef() );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
}


QList< Tomahawk::PlaybackLog >
Track::playbackHistory( const Tomahawk::source_ptr& source ) const
{
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
Track::setPlaybackHistory( const QList< Tomahawk::PlaybackLog >& playbackData )
{
    m_playbackHistory = playbackData;
    emit statsLoaded();
}


unsigned int
Track::playbackCount( const source_ptr& source )
{
    unsigned int count = 0;
    foreach ( const PlaybackLog& log, m_playbackHistory )
    {
        if ( source.isNull() || log.source == source )
            count++;
    }

    return count;
}


void
Track::loadSocialActions()
{
    if ( m_socialActionsLoaded )
        return;

    m_socialActionsLoaded = true;

    DatabaseCommand_LoadSocialActions* cmd = new DatabaseCommand_LoadSocialActions( m_ownRef.toStrongRef() );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
}


void
Track::setAllSocialActions( const QList< SocialAction >& socialActions )
{
    m_allSocialActions = socialActions;
    parseSocialActions();

    emit socialActionsLoaded();
}


QList< SocialAction >
Track::allSocialActions() const
{
    return m_allSocialActions;
}


void
Track::parseSocialActions()
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
Track::loved()
{
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
Track::setLoved( bool loved )
{
    m_currentSocialActions[ "Love" ] = loved;

    QVariantMap loveInfo;
    Tomahawk::InfoSystem::InfoStringHash trackInfo;
    trackInfo["title"] = track();
    trackInfo["artist"] = artist();
    trackInfo["album"] = album();

    loveInfo[ "trackinfo" ] = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );

    Tomahawk::InfoSystem::InfoPushData pushData ( id(),
                                                  ( loved ? Tomahawk::InfoSystem::InfoLove : Tomahawk::InfoSystem::InfoUnLove ),
                                                  loveInfo,
                                                  Tomahawk::InfoSystem::PushShortUrlFlag );

    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( pushData );

    DatabaseCommand_SocialAction* cmd = new DatabaseCommand_SocialAction( m_ownRef.toStrongRef(), QString( "Love" ), loved ? QString( "true" ) : QString( "false" ) );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );

    emit socialActionsLoaded();
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
Track::lyrics() const
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
Track::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
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
            Pipeline::instance()->resolve( m_similarTracks );

            m_simTracksLoaded = true;
            emit similarTracksLoaded();

            break;
        }

        default:
            Q_ASSERT( false );
    }
}


void
Track::infoSystemFinished( QString target )
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

    emit updated();
}


QString
Track::id() const
{
    if ( m_uuid.isEmpty() )
    {
        m_uuid = uuid();
    }

    return m_uuid;
}
