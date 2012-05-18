/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "Query.h"

#include <QtAlgorithms>

#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "database/DatabaseCommand_LogPlayback.h"
#include "database/DatabaseCommand_LoadPlaylistEntries.h"
#include "database/DatabaseCommand_LoadSocialActions.h"
#include "database/DatabaseCommand_SocialAction.h"
#include "database/DatabaseCommand_TrackStats.h"
#include "Album.h"
#include "Collection.h"
#include "Pipeline.h"
#include "Resolver.h"
#include "SourceList.h"
#include "audio/AudioEngine.h"

#include "utils/Logger.h"

using namespace Tomahawk;

SocialAction::SocialAction() {}
SocialAction::~SocialAction() {}

SocialAction& SocialAction::operator=( const SocialAction& other )
{
    action = other.action;
    value = other.value;
    timestamp = other.timestamp;
    source = other.source;

    return *this;
}

SocialAction::SocialAction( const SocialAction& other )
{
    *this = other;
}

PlaybackLog::PlaybackLog() {}
PlaybackLog::~PlaybackLog() {}

PlaybackLog& PlaybackLog::operator=( const PlaybackLog& other )
{
    source = other.source;
    timestamp = other.timestamp;
    secsPlayed = other.secsPlayed;

    return *this;
}

PlaybackLog::PlaybackLog( const PlaybackLog& other )
{
    *this = other;
}

query_ptr
Query::get( const QString& artist, const QString& track, const QString& album, const QID& qid, bool autoResolve )
{
    if ( qid.isEmpty() )
        autoResolve = false;

    query_ptr q = query_ptr( new Query( artist, track, album, qid, autoResolve ), &QObject::deleteLater );
    q->setWeakRef( q.toWeakRef() );

    if ( autoResolve )
        Pipeline::instance()->resolve( q );

    return q;
}


query_ptr
Query::get( const QString& query, const QID& qid )
{
    query_ptr q = query_ptr( new Query( query, qid ), &QObject::deleteLater );
    q->setWeakRef( q.toWeakRef() );

    if ( !qid.isEmpty() )
        Pipeline::instance()->resolve( q );

    return q;
}


Query::Query( const QString& artist, const QString& track, const QString& album, const QID& qid, bool autoResolve )
    : m_qid( qid )
    , m_artist( artist )
    , m_album( album )
    , m_track( track )
    , m_socialActionsLoaded( false )
    , m_simTracksLoaded( false )
    , m_infoJobs( 0 )
{
    init();

    if ( autoResolve )
    {
        connect( Database::instance(), SIGNAL( indexReady() ), SLOT( refreshResults() ), Qt::QueuedConnection );
    }

    connect( Pipeline::instance(), SIGNAL( resolverAdded( Resolver* ) ),
             SLOT( onResolverAdded() ), Qt::QueuedConnection );
    connect( Pipeline::instance(), SIGNAL( resolverRemoved( Resolver* ) ),
             SLOT( onResolverRemoved() ), Qt::QueuedConnection );
}


Query::Query( const QString& query, const QID& qid )
    : m_qid( qid )
    , m_fullTextQuery( query )
{
    init();

    if ( !qid.isEmpty() )
    {
        connect( Database::instance(), SIGNAL( indexReady() ), SLOT( refreshResults() ), Qt::QueuedConnection );
    }
}


Query::~Query()
{
    QMutexLocker lock( &m_mutex );
    m_ownRef.clear();
    m_results.clear();
}


void
Query::init()
{
    m_resolveFinished = false;
    m_solved = false;
    m_playable = false;
    m_duration = -1;
    m_albumpos = 0;
    m_discnumber = 0;

    updateSortNames();
}


void
Query::updateSortNames()
{
    if ( isFullTextQuery() )
    {
        m_artistSortname = DatabaseImpl::sortname( m_fullTextQuery, true );
        m_composerSortName = DatabaseImpl::sortname( m_composer, true );
        m_albumSortname = DatabaseImpl::sortname( m_fullTextQuery );
        m_trackSortname = m_albumSortname;
    }
    else
    {
        m_artistSortname = DatabaseImpl::sortname( m_artist, true );
        m_composerSortName = DatabaseImpl::sortname( m_composer, true );
        m_albumSortname = DatabaseImpl::sortname( m_album );
        m_trackSortname = DatabaseImpl::sortname( m_track );
    }
}


void
Query::addResults( const QList< Tomahawk::result_ptr >& newresults )
{
    {
        QMutexLocker lock( &m_mutex );

/*        const QStringList smt = AudioEngine::instance()->supportedMimeTypes();
        foreach ( const Tomahawk::result_ptr& result, newresults )
        {
            if ( !smt.contains( result->mimetype() ) )
            {
                tDebug() << "Won't accept result, unsupported mimetype" << result->toString() << result->mimetype();
            }
            else
                m_results.append( result );
        }*/

        m_results << newresults;
        qStableSort( m_results.begin(), m_results.end(), Query::resultSorter );
        query_ptr q = m_ownRef.toStrongRef();

        // hook up signals, and check solved status
        foreach( const result_ptr& rp, newresults )
        {
            connect( rp.data(), SIGNAL( statusChanged() ), SLOT( onResultStatusChanged() ) );
        }
    }

    checkResults();
    emit resultsAdded( newresults );
}


void
Query::addAlbums( const QList< Tomahawk::album_ptr >& newalbums )
{
    {
        QMutexLocker lock( &m_mutex );
        m_albums << newalbums;
    }

    emit albumsAdded( newalbums );
}


void
Query::addArtists( const QList< Tomahawk::artist_ptr >& newartists )
{
    {
        QMutexLocker lock( &m_mutex );
        m_artists << newartists;
    }

    emit artistsAdded( newartists );
}


void
Query::refreshResults()
{
    if ( m_resolveFinished )
    {
        m_resolveFinished = false;
        query_ptr q = m_ownRef.toStrongRef();
        if ( q )
            Pipeline::instance()->resolve( q );
    }
}


void
Query::onResultStatusChanged()
{
    {
        QMutexLocker lock( &m_mutex );
        if ( m_results.count() )
            qStableSort( m_results.begin(), m_results.end(), Query::resultSorter );
    }

    checkResults();
    emit resultsChanged();
}


void
Query::removeResult( const Tomahawk::result_ptr& result )
{
    {
        QMutexLocker lock( &m_mutex );
        m_results.removeAll( result );
    }

    emit resultsRemoved( result );
    checkResults();
}


void
Query::onResolvingFinished()
{
    tDebug( LOGVERBOSE ) << "Finished resolving:" << toString();
    if ( !m_resolveFinished )
    {
        m_resolveFinished = true;
        m_resolvers.clear();

        emit resolvingFinished( m_solved );
    }
}


void
Query::onResolverAdded()
{
    if ( !solved() )
    {
        refreshResults();
    }
}


void
Query::onResolverRemoved()
{
    if ( !solved() )
    {
        refreshResults();
    }
}


QList< result_ptr >
Query::results() const
{
    QMutexLocker lock( &m_mutex );
    return m_results;
}


unsigned int
Query::numResults() const
{
    QMutexLocker lock( &m_mutex );
    return m_results.length();
}


QID
Query::id() const
{
    if ( m_qid.isEmpty() )
    {
        m_qid = uuid();
    }

    return m_qid;
}


void
Query::setPlayedBy( const Tomahawk::source_ptr& source, unsigned int playtime )
{
    m_playedBy.first = source;
    m_playedBy.second = playtime;
}


bool
Query::resultSorter( const result_ptr& left, const result_ptr& right )
{
    const float ls = left->score();
    const float rs = right->score();

    if ( ls == rs )
    {
        if ( !left->collection().isNull() && left->collection()->source()->isLocal() )
            return true;
        else
            return false;
    }

    return ls > rs;
}


void
Query::setCurrentResolver( Tomahawk::Resolver* resolver )
{
    m_resolvers << resolver;
}


Tomahawk::Resolver*
Query::currentResolver() const
{
    int x = m_resolvers.count();
    while ( --x )
    {
        QWeakPointer< Resolver > r = m_resolvers.at( x );
        if ( r.isNull() )
            continue;

        return r.data();
    }

    return 0;
}


void
Query::clearResults()
{
    foreach( const result_ptr& rp, m_results )
    {
        removeResult( rp );
    }
}


void
Query::checkResults()
{
    bool playable = false;
    bool solved = false;

    {
        QMutexLocker lock( &m_mutex );

        // hook up signals, and check solved status
        foreach( const result_ptr& rp, m_results )
        {
            if ( rp->score() > 0.0 && rp->collection().isNull() )
            {
                playable = true;
            }
            else if ( !rp->collection().isNull() && rp->collection()->source()->isOnline() )
            {
                playable = true;
            }

            if ( rp->score() > 0.99 )
            {
                solved = true;
            }

            if ( playable )
                break;
        }
    }

    if ( m_playable && !playable )
    {
        refreshResults();
    }

    if ( m_playable != playable )
    {
        m_playable = playable;
        emit playableStateChanged( m_playable );
    }
    if ( m_solved != solved )
    {
        m_solved = solved;
        emit solvedStateChanged( m_solved );
    }
}


bool
Query::equals( const Tomahawk::query_ptr& other ) const
{
    if ( other.isNull() )
        return false;

    return ( artist() == other->artist() &&
             album() == other->album() &&
             track() == other->track() );
}


QVariant
Query::toVariant() const
{
    QVariantMap m;
    m.insert( "artist", artist() );
    m.insert( "album", album() );
    m.insert( "track", track() );
    m.insert( "duration", duration() );
    m.insert( "qid", id() );

    return m;
}


QString
Query::toString() const
{
    if ( !isFullTextQuery() )
        return QString( "Query(%1, %2 - %3)" ).arg( id() ).arg( artist() ).arg( track() );
    else
        return QString( "Query(%1, Fulltext: %2)" ).arg( id() ).arg( fullTextQuery() );
}


// TODO make clever (ft. featuring live (stuff) etc)
float
Query::howSimilar( const Tomahawk::result_ptr& r )
{
    // result values
    const QString rArtistname = r->artist()->sortname();
    const QString rAlbumname  = DatabaseImpl::sortname( r->album()->name() );
    const QString rTrackname  = DatabaseImpl::sortname( r->track() );

    // normal edit distance
    int artdist = levenshtein( m_artistSortname, rArtistname );
    int albdist = levenshtein( m_albumSortname, rAlbumname );
    int trkdist = levenshtein( m_trackSortname, rTrackname );

    // max length of name
    int mlart = qMax( m_artistSortname.length(), rArtistname.length() );
    int mlalb = qMax( m_albumSortname.length(), rAlbumname.length() );
    int mltrk = qMax( m_trackSortname.length(), rTrackname.length() );

    // distance scores
    float dcart = (float)( mlart - artdist ) / mlart;
    float dcalb = (float)( mlalb - albdist ) / mlalb;
    float dctrk = (float)( mltrk - trkdist ) / mltrk;

    if ( isFullTextQuery() )
    {
        const QString artistTrackname = DatabaseImpl::sortname( fullTextQuery() );
        const QString rArtistTrackname  = DatabaseImpl::sortname( r->artist()->name() + " " + r->track() );

        int atrdist = levenshtein( artistTrackname, rArtistTrackname );
        int mlatr = qMax( artistTrackname.length(), rArtistTrackname.length() );
        float dcatr = (float)( mlatr - atrdist ) / mlatr;

        float res = qMax( dcart, dcalb );
        res = qMax( res, dcatr );
        return qMax( res, dctrk );
    }
    else
    {
        // don't penalize for missing album name
        if ( m_albumSortname.isEmpty() )
            dcalb = 1.0;

        // weighted, so album match is worth less than track title
        float combined = ( dcart * 4 + dcalb + dctrk * 5 ) / 10;
        return combined;
    }
}


QPair< Tomahawk::source_ptr, unsigned int >
Query::playedBy() const
{
    return m_playedBy;
}


void
Query::loadStats()
{
    query_ptr q = m_ownRef.toStrongRef();

    DatabaseCommand_TrackStats* cmd = new DatabaseCommand_TrackStats( q );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
}


QList< Tomahawk::PlaybackLog >
Query::playbackHistory( const Tomahawk::source_ptr& source ) const
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
Query::setPlaybackHistory( const QList< Tomahawk::PlaybackLog >& playbackData )
{
    m_playbackHistory = playbackData;
    emit statsLoaded();
}


unsigned int
Query::playbackCount( const source_ptr& source )
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
Query::loadSocialActions()
{
    m_socialActionsLoaded = true;
    query_ptr q = m_ownRef.toStrongRef();

    DatabaseCommand_LoadSocialActions* cmd = new DatabaseCommand_LoadSocialActions( q );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
}


void
Query::setAllSocialActions( const QList< SocialAction >& socialActions )
{
    m_allSocialActions = socialActions;
    parseSocialActions();

    emit socialActionsLoaded();
}


QList< SocialAction >
Query::allSocialActions() const
{
    return m_allSocialActions;
}


void
Query::parseSocialActions()
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
Query::loved()
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
Query::setLoved( bool loved )
{
    query_ptr q = m_ownRef.toStrongRef();
    if ( q )
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

        DatabaseCommand_SocialAction* cmd = new DatabaseCommand_SocialAction( q, QString( "Love" ), loved ? QString( "true" ) : QString( "false" ) );
        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );

        emit socialActionsLoaded();
    }
}


QString
Query::socialActionDescription( const QString& action, DescriptionMode mode ) const
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
        }
    }
    if ( loveCounter > 0 )
    {
        if ( loveCounter > 3 )
            desc += " " + tr( "and" ) + " <b>" + tr( "%n other(s)", "", loveCounter - 3 ) + "</b>";

        if ( mode == Short )
            desc = "<b>" + tr( "%1 people" ).arg( loveCounter ) + "</b>";

        desc += " " + tr( "loved this track" ); //FIXME: more action descs required
    }

    return desc;
}


#ifndef ENABLE_HEADLESS
QPixmap
Query::cover( const QSize& size, bool forceLoad ) const
{
    if ( m_albumPtr.isNull() )
    {
        m_artistPtr = Artist::get( artist(), false );
        m_albumPtr = Album::get( m_artistPtr, album(), false );
        connect( m_artistPtr.data(), SIGNAL( updated() ), SIGNAL( updated() ), Qt::UniqueConnection );
        connect( m_artistPtr.data(), SIGNAL( coverChanged() ), SIGNAL( coverChanged() ), Qt::UniqueConnection );
        connect( m_albumPtr.data(), SIGNAL( updated() ), SIGNAL( updated() ), Qt::UniqueConnection );
        connect( m_albumPtr.data(), SIGNAL( coverChanged() ), SIGNAL( coverChanged() ), Qt::UniqueConnection );
    }

    m_albumPtr->cover( size, forceLoad );
    if ( m_albumPtr->infoLoaded() )
    {
        if ( !m_albumPtr->cover( size ).isNull() )
            return m_albumPtr->cover( size );

        return m_artistPtr->cover( size, forceLoad );
    }

    return QPixmap();
}
#endif


QList<Tomahawk::query_ptr>
Query::similarTracks() const
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


void
Query::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != id() )
        return;

    QVariantMap returnedData = output.value< QVariantMap >();
    switch ( requestData.type )
    {
        case InfoSystem::InfoTrackSimilars:
        {
            const QStringList artists = returnedData["artists"].toStringList();
            const QStringList tracks = returnedData["tracks"].toStringList();

            for ( int i = 0; i < tracks.count() && i < 50; i++ )
            {
                m_similarTracks << Query::get( artists.at( i ), tracks.at( i ), QString(), uuid(), true );
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
Query::infoSystemFinished( QString target )
{
    tDebug() << Q_FUNC_INFO;
    Q_UNUSED( target );

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


int
Query::levenshtein( const QString& source, const QString& target )
{
    // Step 1
    const int n = source.length();
    const int m = target.length();

    if ( n == 0 )
        return m;
    if ( m == 0 )
        return n;

    // Good form to declare a TYPEDEF
    typedef QVector< QVector<int> > Tmatrix;
    Tmatrix matrix;
    matrix.resize( n + 1 );

    // Size the vectors in the 2.nd dimension. Unfortunately C++ doesn't
    // allow for allocation on declaration of 2.nd dimension of vec of vec
    for ( int i = 0; i <= n; i++ )
    {
        QVector<int> tmp;
        tmp.resize( m + 1 );
        matrix.insert( i, tmp );
    }

    // Step 2
    for ( int i = 0; i <= n; i++ )
        matrix[i][0] = i;
    for ( int j = 0; j <= m; j++ )
        matrix[0][j] = j;

    // Step 3
    for ( int i = 1; i <= n; i++ )
    {
        const QChar s_i = source[i - 1];

        // Step 4
        for ( int j = 1; j <= m; j++ )
        {
            const QChar t_j = target[j - 1];

            // Step 5
            int cost;
            if ( s_i == t_j )
                cost = 0;
            else
                cost = 1;

            // Step 6
            const int above = matrix[i - 1][j];
            const int left = matrix[i][j - 1];
            const int diag = matrix[i - 1][j - 1];

            int cell = ( ( ( left + 1 ) > ( diag + cost ) ) ? diag + cost : left + 1 );
            if ( above + 1 < cell )
                cell = above + 1;

            // Step 6A: Cover transposition, in addition to deletion,
            // insertion and substitution. This step is taken from:
            // Berghel, Hal ; Roach, David : "An Extension of Ukkonen's
            // Enhanced Dynamic Programming ASM Algorithm"
            // (http://www.acm.org/~hlb/publications/asm/asm.html)
            if ( i > 2 && j > 2 )
            {
                int trans = matrix[i - 2][j - 2] + 1;

                if ( source[ i - 2 ] != t_j ) trans++;
                if ( s_i != target[ j - 2 ] ) trans++;
                if ( cell > trans ) cell = trans;
            }
            matrix[i][j] = cell;
        }
    }

    // Step 7
    return matrix[n][m];
}
