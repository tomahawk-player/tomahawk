/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "Result.h"

#include "Album.h"
#include "Track.h"
#include "collection/Collection.h"
#include "resolvers/Resolver.h"
#include "Source.h"
#include "Pipeline.h"
#include "database/Database.h"
#include "database/DatabaseCommand_Resolve.h"
#include "database/DatabaseCommand_AllTracks.h"
#include "database/DatabaseCommand_AddFiles.h"
#include "filemetadata/MetadataEditor.h"

#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "resolvers/ExternalResolverGui.h"

#include "result_wptr.h"

using namespace Tomahawk;

static QHash< QString, result_wptr > s_results;
static QMutex s_mutex;

typedef QMap< QString, QPixmap > SourceIconCache;
Q_GLOBAL_STATIC( SourceIconCache, sourceIconCache );
static QMutex s_sourceIconMutex;

inline QString
sourceCacheKey( Resolver* resolver, const QSize& size, TomahawkUtils::ImageMode style )
{
    QString str;
    QTextStream stream( &str );
    stream << resolver << size.width() << size.height() << "_" << style;
    return str;
}


Tomahawk::result_ptr
Result::get( const QString& url )
{
    if ( url.trimmed().isEmpty() )
    {
//        Q_ASSERT( false );
        return result_ptr();
    }

    QMutexLocker lock( &s_mutex );
    if ( s_results.contains( url ) )
    {
        return s_results.value( url );
    }

    result_ptr r = result_ptr( new Result( url ), &Result::deleteLater );
    s_results.insert( url, r );

    return r;
}


bool
Result::isCached( const QString& url )
{
    QMutexLocker lock( &s_mutex );
    return ( s_results.contains( url ) );
}


Result::Result( const QString& url )
    : QObject()
    , m_url( url )
    , m_bitrate( 0 )
    , m_size( 0 )
    , m_modtime( 0 )
    , m_score( 0 )
    , m_fileId( 0 )
    , m_checked( false )
{
    connect( Pipeline::instance(), SIGNAL( resolverRemoved( Tomahawk::Resolver* ) ), SLOT( onResolverRemoved( Tomahawk::Resolver* ) ), Qt::QueuedConnection );
}


Result::~Result()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << toString();
}


void
Result::deleteLater()
{
    QMutexLocker lock( &s_mutex );

    if ( s_results.contains( m_url ) )
    {
        s_results.remove( m_url );
    }

    QObject::deleteLater();
}


bool
Result::isValid() const
{
    return m_track && !m_track->artist().isEmpty() && !m_track->track().isEmpty();
}


void
Result::onResolverRemoved( Tomahawk::Resolver* resolver )
{
    if ( m_resolvedBy.data() == resolver )
    {
        m_resolvedBy = 0;
        emit statusChanged();
    }
}


collection_ptr
Result::collection() const
{
    return m_collection;
}


float
Result::score() const
{
    if ( isOnline() )
        return m_score;
    else
        return 0.0;
}


RID
Result::id() const
{
    if ( m_rid.isEmpty() )
        m_rid = uuid();

    return m_rid;
}


bool
Result::isOnline() const
{
    if ( !collection().isNull() )
    {
        return collection()->source()->isOnline();
    }
    else
    {
        return !m_resolvedBy.isNull();
    }
}


bool
Result::playable() const
{
    if ( collection() )
    {
        return collection()->source()->isOnline();
    }
    else
    {
        return score() > 0.0;
    }
}


QVariant
Result::toVariant() const
{
    QVariantMap m;
    m.insert( "artist", m_track->artist() );
    m.insert( "album", m_track->album() );
    m.insert( "track", m_track->track() );
    m.insert( "source", friendlySource() );
    m.insert( "mimetype", mimetype() );
    m.insert( "size", size() );
    m.insert( "bitrate", bitrate() );
    m.insert( "duration", m_track->duration() );
    m.insert( "score", score() );
    m.insert( "sid", id() );
    m.insert( "discnumber", m_track->discnumber() );
    m.insert( "albumpos", m_track->albumpos() );

    if ( !m_track->composer().isEmpty() )
        m.insert( "composer", m_track->composer() );

    return m;
}


QString
Result::toString() const
{
    return QString( "Result(%1, score: %2) %3 - %4%5 (%6)" )
              .arg( id() )
              .arg( score() )
              .arg( track()->artist() )
              .arg( track()->track() )
              .arg( track()->album().isEmpty() ? "" : QString( " on %1" ).arg( track()->album() ) )
              .arg( url() );
}


Tomahawk::query_ptr
Result::toQuery()
{
    if ( m_query.isNull() )
    {
        query_ptr query = Tomahawk::Query::get( m_track );
        if ( !query )
            return query_ptr();

        m_query = query->weakRef();

        QList<Tomahawk::result_ptr> rl;
        rl << Result::get( m_url );

        query->addResults( rl );
        query->setResolveFinished( true );
        return query;
    }

    return m_query.toStrongRef();
}


void
Result::onOnline()
{
    emit statusChanged();
}


void
Result::onOffline()
{
    emit statusChanged();
}


void
Result::setCollection( const Tomahawk::collection_ptr& collection )
{
    m_collection = collection;
    connect( m_collection->source().data(), SIGNAL( online() ), SLOT( onOnline() ), Qt::QueuedConnection );
    connect( m_collection->source().data(), SIGNAL( offline() ), SLOT( onOffline() ), Qt::QueuedConnection );
}


QString
Result::friendlySource() const
{
    if ( collection().isNull() )
    {
        return m_friendlySource;
    }
    else
        return collection()->source()->friendlyName();
}


QPixmap
Result::sourceIcon( TomahawkUtils::ImageMode style, const QSize& desiredSize ) const
{
    if ( collection().isNull() )
    {
        const ExternalResolverGui* guiResolver = qobject_cast< ExternalResolverGui* >( m_resolvedBy.data() );
        if ( !guiResolver )
        {
            return QPixmap();
        }
        else
        {
            QMutexLocker l( &s_sourceIconMutex );

            const QString key = sourceCacheKey( m_resolvedBy.data(), desiredSize, style );
            if ( !sourceIconCache()->contains( key ) )
            {
                QPixmap pixmap = guiResolver->icon();
                if ( !desiredSize.isEmpty() )
                    pixmap = pixmap.scaled( desiredSize, Qt::KeepAspectRatio, Qt::SmoothTransformation );

                switch ( style )
                {
                    case TomahawkUtils::DropShadow:
                        pixmap = TomahawkUtils::addDropShadow( pixmap, QSize() );
                        break;

                    case TomahawkUtils::RoundedCorners:
                        pixmap = TomahawkUtils::createRoundedImage( pixmap, QSize() );
                        break;

                    default:
                        break;
                }

                sourceIconCache()->insert( key, pixmap );
                return pixmap;
            }
            else
            {
                return sourceIconCache()->value( key );
            }
        }
    }
    else
    {
        QPixmap avatar = collection()->source()->avatar( TomahawkUtils::RoundedCorners, desiredSize );
        if ( !avatar )
        {
            avatar = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultSourceAvatar, TomahawkUtils::RoundedCorners, desiredSize );
        }
        return avatar;
    }
}


Tomahawk::Resolver*
Result::resolvedBy() const
{
    if ( m_resolvedBy.isNull() )
        return 0;

    return m_resolvedBy.data();
}


void
Result::setResolvedBy( Tomahawk::Resolver* resolver )
{
    m_resolvedBy = QPointer< Tomahawk::Resolver >( resolver );
}


void
Result::doneEditing()
{
//    m_query.clear();
    emit updated();
}


track_ptr
Result::track() const
{
    return m_track;
}
