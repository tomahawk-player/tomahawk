/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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
#include "resolvers/Resolver.h"
#include "utils/Logger.h"

#include "Album.h"
#include "Pipeline.h"
#include "Result.h"

#include <QtAlgorithms>
#include <QDebug>
#include <QCoreApplication>

using namespace Tomahawk;


query_ptr
Query::get( const QString& artist, const QString& track, const QString& album, const QID& qid, bool autoResolve )
{
    if ( artist.trimmed().isEmpty() || track.trimmed().isEmpty() )
        return query_ptr();

    if ( qid.isEmpty() )
        autoResolve = false;

    query_ptr q = query_ptr( new Query( Track::get( artist, track, album ), qid, autoResolve ), &QObject::deleteLater );
    q->moveToThread( QCoreApplication::instance()->thread() );
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


query_ptr
Query::getFixed( const track_ptr& track, const result_ptr& result )
{
    query_ptr q = query_ptr( new Query( track, result ), &QObject::deleteLater );
    q->setWeakRef( q.toWeakRef() );

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


Query::Query( const track_ptr& track, const result_ptr& result )
    : d_ptr( new QueryPrivate( this, track, QString() ) )
{
    Q_D( Query );

    init();
    d->allowReresolve = false;
    d->resolveFinished = true;
    d->results << result;
    d->playable = result->playable();
    d->solved = true;
    d->score = 1.0;
    connect( result.data(), SIGNAL( statusChanged() ), SLOT( onResultStatusChanged() ) );
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
}


void
Query::init()
{
    Q_D( Query );
    d->resolveFinished = false;
    d->solved = false;
    d->playable = false;
    d->saveResultHint = false;
    d->score = 0.0;
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

    {
        QMutexLocker lock( &d->mutex );
        if ( !d->results.isEmpty() )
            return d->results.first()->track();
    }

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
        sortResults();

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

    clearResults();
    if ( d->resolveFinished && d->allowReresolve )
    {
        d->resolveFinished = false;
        query_ptr q = d->ownRef.toStrongRef();
        if ( q )
            Pipeline::instance()->resolve( q );
    }
}


void
Query::onResultStatusChanged()
{
    {
        Q_D( Query );
        QMutexLocker lock( &d->mutex );
        if ( !d->results.isEmpty() )
            sortResults();
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
        if ( d->preferredResult == result )
        {
            d->preferredResult.clear();
        }
        sortResults();
    }

    emit resultsRemoved( result );
    checkResults();
    emit resultsChanged();
}


void
Query::clearResults()
{
    Q_D( Query );

    d->solved = false;
    d->playable = false;

    {
        QMutexLocker lock( &d->mutex );
        d->results.clear();
    }

    emit playableStateChanged( false );
    emit solvedStateChanged( false );
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
Query::numResults( bool onlyPlayableResults ) const
{
    Q_D( const Query );
    QMutexLocker lock( &d->mutex );

    if ( onlyPlayableResults )
    {
        unsigned int c = 0;
        foreach ( const result_ptr& result, d->results )
        {
            if ( result->isOnline() )
                c++;
        }

        return c;
    }

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
    Q_D( Query );
    if ( !d->preferredResult.isNull() )
    {
        if ( d->preferredResult == left )
            return true;
        if ( d->preferredResult == right )
            return false;
    }

    const float ls = left->isOnline() ? howSimilar( left ) : 0.0;
    const float rs = right->isOnline() ? howSimilar( right ) : 0.0;

    if ( ls == rs )
    {
        if ( right->isLocal() )
        {
            return false;
        }
        if ( left->isPreview() != right->isPreview() )
        {
            return !left->isPreview();
        }

        if ( left->resolvedBy() != nullptr && right->resolvedBy() != nullptr )
        {
            return left->resolvedBy()->weight() > right->resolvedBy()->weight();
        }

        return left->id() > right->id();
    }

    if ( left->isPreview() != right->isPreview() )
    {
        return !left->isPreview();
    }

    return ls > rs;
}


result_ptr
Query::preferredResult() const
{
    Q_D( const Query );
    return d->preferredResult;
}


void
Query::setPreferredResult( const result_ptr& result )
{
    {
        Q_D( Query );
        QMutexLocker lock( &d->mutex );

        Q_ASSERT( d->results.contains( result ) );
        d->preferredResult = result;
        sortResults();
    }

    emit resultsChanged();
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
Query::allowReresolve()
{
    Q_D( Query );
    d->allowReresolve = true;
}


void
Query::disallowReresolve()
{
    Q_D( Query );
    d->allowReresolve = false;
}


void
Query::checkResults()
{
    Q_D( Query );
    if ( !d->results.isEmpty() )
    {
        d->score = howSimilar( d->results.first() );
    }
    else
    {
        d->score = 0.0;
    }

    bool playable = false;
    bool solved = false;

    {
        QMutexLocker lock( &d->mutex );

        // hook up signals, and check solved status
        foreach( const result_ptr& rp, d->results )
        {
            if ( rp->playable() )
                playable = true;

            if ( rp->isOnline() && howSimilar( rp ) > 0.99 )
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
    else
    {
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
}


bool
Query::equals( const Tomahawk::query_ptr& other, bool ignoreCase, bool ignoreAlbum ) const
{
    if ( !other )
        return false;

    if ( ignoreCase )
    {
        return ( queryTrack()->artist().toLower() == other->queryTrack()->artist().toLower() &&
                 ( ignoreAlbum || queryTrack()->album().toLower() == other->queryTrack()->album().toLower() ) &&
                   queryTrack()->track().toLower() == other->queryTrack()->track().toLower() );
    }

    return ( queryTrack()->artist() == other->queryTrack()->artist() &&
             ( ignoreAlbum || queryTrack()->album() == other->queryTrack()->album() ) &&
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


float
Query::score() const
{
    Q_D( const Query );
    return d->score;
}


// TODO make clever (ft. featuring live (stuff) etc)
float
Query::howSimilar( const Tomahawk::result_ptr& r )
{
    Q_D( Query );
    if (d->howSimilarCache.find(r->id()) != d->howSimilarCache.end())
    {
        return d->howSimilarCache[r->id()];
    }
    // result values
    const QString& rArtistname = r->track()->artistSortname();
    const QString& rAlbumname  = r->track()->albumSortname();
    const QString& rTrackname  = r->track()->trackSortname();
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

    static const QRegExp filterOutChars = QRegExp(QString::fromUtf8("[-`´~!@#$%^&*()_—+=|:;<>«»,.?/{}\'\"\\[\\]\\\\]"));

    //Cleanup symbols for minor naming differences
    qArtistname.remove(filterOutChars);
    qTrackname.remove(filterOutChars);
    qAlbumname.remove(filterOutChars);

    // normal edit distance
    const int artdist = TomahawkUtils::levenshtein( qArtistname, rArtistname );
    const int trkdist = TomahawkUtils::levenshtein( qTrackname, rTrackname );

    // max length of name
    const int mlart = qMax( qArtistname.length(), rArtistname.length() );
    const int mltrk = qMax( qTrackname.length(), rTrackname.length() );

    // distance scores
    const float dcart = (float)( mlart - artdist ) / mlart;
    const float dctrk = (float)( mltrk - trkdist ) / mltrk;

    // don't penalize for missing album name
    float dcalb = 1.0;
    if ( !qAlbumname.isEmpty() )
    {
        const int albdist = TomahawkUtils::levenshtein( qAlbumname, rAlbumname );
        const int mlalb = qMax( qAlbumname.length(), rAlbumname.length() );
        dcalb = (float)( mlalb - albdist ) / mlalb;
    }

    if ( isFullTextQuery() )
    {
        const QString artistTrackname = DatabaseImpl::sortname( fullTextQuery() );
        const QString rArtistTrackname = DatabaseImpl::sortname( r->track()->artist() + " " + r->track()->track() );

        const int atrdist = TomahawkUtils::levenshtein( artistTrackname, rArtistTrackname );
        const int mlatr = qMax( artistTrackname.length(), rArtistTrackname.length() );
        const float dcatr = (float)( mlatr - atrdist ) / mlatr;

        float resultScore = qMax( dctrk, qMax( dcatr, qMax( dcart, dcalb ) ) );
        d->howSimilarCache[r->id()] =  resultScore;
        return resultScore;
    }
    else
    {
        // weighted, so album match is worth less than track title
        float resultScore = ( dcart * 4 + dcalb + dctrk * 5 ) / 10;
        d->howSimilarCache[r->id()] =  resultScore;
        return resultScore;
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


void
Query::sortResults()
{
    Q_D( Query );
    qStableSort( d->results.begin(), d->results.end(), std::bind( &Query::resultSorter, this, std::placeholders::_1, std::placeholders::_2 ) );
}
