/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#include "Query_p.h"

#include "audio/AudioEngine.h"
#include "collection/Collection.h"
#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "database/DatabaseCommand_LogPlayback.h"
#include "database/DatabaseCommand_LoadPlaylistEntries.h"
#include "database/DatabaseCommand_LoadSocialActions.h"
#include "database/DatabaseCommand_SocialAction.h"
#include "database/DatabaseCommand_TrackStats.h"
#include "resolvers/Resolver.h"
#include "utils/Logger.h"

#include "Album.h"
#include "Pipeline.h"
#include "Result.h"
#include "SourceList.h"

#include <QtAlgorithms>
#include <QDebug>

using namespace Tomahawk;


query_ptr
Query::get( const QString& artist, const QString& track, const QString& album, const QID& qid, bool autoResolve )
{
    if ( artist.trimmed().isEmpty() || track.trimmed().isEmpty() )
        return query_ptr();

    if ( qid.isEmpty() )
        autoResolve = false;

    query_ptr q = query_ptr( new Query( Track::get( artist, track, album ), qid, autoResolve ), &QObject::deleteLater );
    q->setWeakRef( q.toWeakRef() );

    if ( autoResolve )
        Pipeline::instance()->resolve( q );

    return q;
}


query_ptr
Query::get( const Tomahawk::track_ptr& track, const QID& qid )
{
    query_ptr q = query_ptr( new Query( track, qid, false ), &QObject::deleteLater );
    q->setWeakRef( q.toWeakRef() );

    return q;
}


query_ptr
Query::get( const QString& query, const QID& qid )
{
    Q_ASSERT( !query.trimmed().isEmpty() );

    query_ptr q = query_ptr( new Query( query, qid ), &QObject::deleteLater );
    q->setWeakRef( q.toWeakRef() );

    if ( !qid.isEmpty() )
        Pipeline::instance()->resolve( q );

    return q;
}


Query::Query( const track_ptr& track, const QID& qid, bool autoResolve )
    : d_ptr( new QueryPrivate( this, track, qid ) )
{
    init();

    if ( autoResolve )
    {
        connect( Database::instance(), SIGNAL( indexReady() ), SLOT( refreshResults() ), Qt::QueuedConnection );
    }

    connect( Pipeline::instance(), SIGNAL( resolverAdded( Tomahawk::Resolver* ) ), SLOT( onResolverAdded() ), Qt::QueuedConnection );
}


Query::Query( const QString& query, const QID& qid )
    : d_ptr( new QueryPrivate( this, query, qid ) )
{
    init();

    if ( !qid.isEmpty() )
    {
        connect( Database::instance(), SIGNAL( indexReady() ), SLOT( refreshResults() ), Qt::QueuedConnection );
    }
}


Query::~Query()
{
    Q_D( Query );
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << toString();

    QMutexLocker lock( &d->mutex );
    d->ownRef.clear();
    d->results.clear();
}


void
Query::init()
{
    Q_D( Query );
    d->resolveFinished = false;
    d->solved = false;
    d->playable = false;
    d->saveResultHint = false;
}


track_ptr
Query::queryTrack() const
{
    Q_D( const Query );
    return d->queryTrack;
}


track_ptr
Query::track() const
{
    Q_D( const Query );
    if ( !results().isEmpty() )
        return results().first()->track();

    return d->queryTrack;
}


void
Query::addResults( const QList< Tomahawk::result_ptr >& newresults )
{
    Q_D( Query );
    {
        QMutexLocker lock( &d->mutex );

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

        d->results << newresults;
        qStableSort( d->results.begin(), d->results.end(), Query::resultSorter );

        // hook up signals, and check solved status
        foreach( const result_ptr& rp, newresults )
        {
            connect( rp.data(), SIGNAL( statusChanged() ), SLOT( onResultStatusChanged() ) );
        }
    }

    checkResults();
    emit resultsAdded( newresults );
    emit resultsChanged();
}


void
Query::addAlbums( const QList< Tomahawk::album_ptr >& newalbums )
{
    {
        Q_D( Query );
        QMutexLocker lock( &d->mutex );
        d->albums << newalbums;
    }

    emit albumsAdded( newalbums );
}


void
Query::addArtists( const QList< Tomahawk::artist_ptr >& newartists )
{
    {
        Q_D( Query );
        QMutexLocker lock( &d->mutex );
        d->artists << newartists;
    }

    emit artistsAdded( newartists );
}


void
Query::refreshResults()
{
    Q_D( Query );
    if ( d->resolveFinished )
    {
        d->resolveFinished = false;
        query_ptr q = d->ownRef.toStrongRef();
        if ( q )
            Pipeline::instance()->resolve( q );
    }
}


Query::Query()
    : d_ptr( new QueryPrivate( this ) )
{
}


void
Query::onResultStatusChanged()
{
    {
        Q_D( Query );
        QMutexLocker lock( &d->mutex );
        if ( d->results.count() )
            qStableSort( d->results.begin(), d->results.end(), Query::resultSorter );
    }

    checkResults();
    emit resultsChanged();
}


void
Query::removeResult( const Tomahawk::result_ptr& result )
{
    {
        Q_D( Query );
        QMutexLocker lock( &d->mutex );
        d->results.removeAll( result );
    }

    emit resultsRemoved( result );
    checkResults();
    emit resultsChanged();
}


void
Query::onResolvingFinished()
{
    Q_D( Query );
    tDebug( LOGVERBOSE ) << "Finished resolving:" << toString();
    if ( !d->resolveFinished )
    {
        d->resolveFinished = true;
        d->resolvers.clear();

        emit resolvingFinished( d->playable );
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


QList< result_ptr >
Query::results() const
{
    Q_D( const Query );
    QMutexLocker lock( &d->mutex );
    return d->results;
}


unsigned int
Query::numResults() const
{
    Q_D( const Query );
    QMutexLocker lock( &d->mutex );
    return d->results.length();
}


bool
Query::resolvingFinished() const
{
    Q_D( const Query );
    return d->resolveFinished;
}


bool
Query::solved() const
{
    Q_D( const Query );
    return d->solved;
}


bool
Query::playable() const
{
    Q_D( const Query );
    return d->playable;
}


QID
Query::id() const
{
    Q_D( const Query );
    if ( d->qid.isEmpty() )
    {
        d->qid = uuid();
    }

    return d->qid;
}


bool
Query::resultSorter( const result_ptr& left, const result_ptr& right )
{
    const float ls = left->isOnline() ? left->score() : 0.0;
    const float rs = right->isOnline() ? right->score() : 0.0;

    if ( ls == rs )
    {
/*        if ( left->collection() && left->collection()->source()->isLocal() )
        {
            return true;
        }*/
        if ( right->collection() && right->collection()->source()->isLocal() )
        {
            return false;
        }

        return true;
    }

    return ls > rs;
}


void
Query::setCurrentResolver( Tomahawk::Resolver* resolver )
{
    Q_D( Query );
    d->resolvers << resolver;
}


Tomahawk::Resolver*
Query::currentResolver() const
{
    Q_D( const Query );
    int x = d->resolvers.count();
    while ( --x )
    {
        QPointer< Resolver > r = d->resolvers.at( x );
        if ( r.isNull() )
            continue;

        return r.data();
    }

    return 0;
}


QList< QPointer<Resolver> >
Query::resolvedBy() const
{
    Q_D( const Query );
    return d->resolvers;
}


QString
Query::fullTextQuery() const
{
    Q_D( const Query );
    return d->fullTextQuery;
}


bool
Query::isFullTextQuery() const
{
    Q_D( const Query );
    return !d->fullTextQuery.isEmpty();
}


void
Query::setResolveFinished( bool resolved )
{
    Q_D( Query );
    d->resolveFinished = resolved;
}


void
Query::clearResults()
{
    foreach( const result_ptr& rp, results() )
    {
        removeResult( rp );
    }
}


void
Query::checkResults()
{
    Q_D( Query );
    bool playable = false;
    bool solved = false;

    {
        QMutexLocker lock( &d->mutex );

        // hook up signals, and check solved status
        foreach( const result_ptr& rp, d->results )
        {
            if ( rp->playable() )
                playable = true;

            if ( rp->isOnline() && rp->score() > 0.99 )
            {
                solved = true;
            }

            if ( playable )
                break;
        }
    }

    if ( d->solved && !solved )
    {
        refreshResults();
    }
    if ( d->playable != playable )
    {
        d->playable = playable;
        emit playableStateChanged( d->playable );
    }
    if ( d->solved != solved )
    {
        d->solved = solved;
        emit solvedStateChanged( d->solved );
    }
}


bool
Query::equals( const Tomahawk::query_ptr& other, bool ignoreCase ) const
{
    if ( other.isNull() )
        return false;

    if ( ignoreCase )
        return ( queryTrack()->artist().toLower() == other->queryTrack()->artist().toLower() &&
                 queryTrack()->album().toLower() == other->queryTrack()->album().toLower() &&
                 queryTrack()->track().toLower() == other->queryTrack()->track().toLower() );
    else
        return ( queryTrack()->artist() == other->queryTrack()->artist() &&
                 queryTrack()->album() == other->queryTrack()->album() &&
                 queryTrack()->track() == other->queryTrack()->track() );
}


QVariant
Query::toVariant() const
{
    QVariantMap m;
    m.insert( "artist", queryTrack()->artist() );
    m.insert( "album", queryTrack()->album() );
    m.insert( "track", queryTrack()->track() );
    m.insert( "duration", queryTrack()->duration() );
    m.insert( "qid", id() );

    return m;
}


QString
Query::toString() const
{
    if ( !isFullTextQuery() )
    {
        return QString( "Query(%1, %2 - %3%4)" )
                  .arg( id() )
                  .arg( queryTrack()->artist() )
                  .arg( queryTrack()->track() )
                  .arg( queryTrack()->album().isEmpty() ? "" : QString( " on %1" ).arg( queryTrack()->album() ) );
    }
    else
    {
        return QString( "Query(%1, Fulltext: %2)" )
                  .arg( id() )
                  .arg( fullTextQuery() );
    }
}


// TODO make clever (ft. featuring live (stuff) etc)
float
Query::howSimilar( const Tomahawk::result_ptr& r )
{
    Q_D( Query );
    // result values
    const QString rArtistname = r->track()->artistSortname();
    const QString rAlbumname  = r->track()->albumSortname();
    const QString rTrackname  = r->track()->trackSortname();

    QString qArtistname;
    QString qAlbumname;
    QString qTrackname;

    if ( isFullTextQuery() )
    {
        qArtistname = DatabaseImpl::sortname( d->fullTextQuery, true );
        qAlbumname = DatabaseImpl::sortname( d->fullTextQuery );
        qTrackname = qAlbumname;
    }
    else
    {
        qArtistname = queryTrack()->artistSortname();
        qAlbumname  = queryTrack()->albumSortname();
        qTrackname  = queryTrack()->trackSortname();
    }

    // normal edit distance
    int artdist = TomahawkUtils::levenshtein( qArtistname, rArtistname );
    int albdist = TomahawkUtils::levenshtein( qAlbumname, rAlbumname );
    int trkdist = TomahawkUtils::levenshtein( qTrackname, rTrackname );

    // max length of name
    int mlart = qMax( qArtistname.length(), rArtistname.length() );
    int mlalb = qMax( qAlbumname.length(), rAlbumname.length() );
    int mltrk = qMax( qTrackname.length(), rTrackname.length() );

    // distance scores
    float dcart = (float)( mlart - artdist ) / mlart;
    float dcalb = (float)( mlalb - albdist ) / mlalb;
    float dctrk = (float)( mltrk - trkdist ) / mltrk;

    if ( isFullTextQuery() )
    {
        const QString artistTrackname = DatabaseImpl::sortname( fullTextQuery() );
        const QString rArtistTrackname  = DatabaseImpl::sortname( r->track()->artist() + " " + r->track()->track() );

        int atrdist = TomahawkUtils::levenshtein( artistTrackname, rArtistTrackname );
        int mlatr = qMax( artistTrackname.length(), rArtistTrackname.length() );
        float dcatr = (float)( mlatr - atrdist ) / mlatr;

        float res = qMax( dcart, dcalb );
        res = qMax( res, dcatr );
        return qMax( res, dctrk );
    }
    else
    {
        // don't penalize for missing album name
        if ( queryTrack()->albumSortname().isEmpty() )
            dcalb = 1.0;

        // weighted, so album match is worth less than track title
        float combined = ( dcart * 4 + dcalb + dctrk * 5 ) / 10;
        return combined;
    }
}


void
Query::setSaveHTTPResultHint( bool saveResultHint )
{
    Q_D( Query );
    d->saveResultHint = saveResultHint;
}


bool
Query::saveHTTPResultHint() const
{
    Q_D( const Query );
    return d->saveResultHint;
}


QString
Query::resultHint() const
{
    Q_D( const Query );
    return d->resultHint;
}


void
Query::setResultHint( const QString& resultHint )
{
    Q_D( Query );
    d->resultHint = resultHint;
}


QWeakPointer<Query>
Query::weakRef()
{
    Q_D( Query );
    return d->ownRef;
}


void
Query::setWeakRef( QWeakPointer<Query> weakRef )
{
    Q_D( Query );
    d->ownRef = weakRef;
}

