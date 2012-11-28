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
#include "Collection.h"
#include "Resolver.h"
#include "Source.h"
#include "Pipeline.h"
#include "database/Database.h"
#include "database/DatabaseCommand_Resolve.h"
#include "database/DatabaseCommand_AllTracks.h"
#include "database/DatabaseCommand_AddFiles.h"
#include "filemetadata/MetadataEditor.h"

#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "ExternalResolverGui.h"

using namespace Tomahawk;

static QHash< QString, QWeakPointer< Result > > s_results;
static QMutex s_mutex;

typedef QMap< QString, QPixmap > SourceIconCache;
Q_GLOBAL_STATIC( SourceIconCache, sourceIconCache );
static QMutex s_sourceIconMutex;

inline QString sourceCacheKey( Resolver* resolver, const QSize& size, TomahawkUtils::ImageMode style )
{
    QString str;
    QTextStream stream( &str );
    stream << resolver << size.width() << size.height() << "_" << style;
    return str;
}


Tomahawk::result_ptr
Result::get( const QString& url )
{
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
    , m_duration( 0 )
    , m_bitrate( 0 )
    , m_size( 0 )
    , m_albumpos( 0 )
    , m_modtime( 0 )
    , m_discnumber( 0 )
    , m_year( 0 )
    , m_score( 0 )
    , m_trackId( 0 )
    , m_fileId( 0 )
{
    connect( Pipeline::instance(), SIGNAL( resolverRemoved( Tomahawk::Resolver* ) ), SLOT( onResolverRemoved( Tomahawk::Resolver* ) ), Qt::QueuedConnection );
}


Result::~Result()
{
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


void
Result::onResolverRemoved( Tomahawk::Resolver* resolver )
{
    if ( m_resolvedBy.data() == resolver )
    {
        m_resolvedBy.clear();
        emit statusChanged();
    }
}


artist_ptr
Result::artist() const
{
    return m_artist;
}


artist_ptr
Result::composer() const
{
    return m_composer;
}


album_ptr
Result::album() const
{
    return m_album;
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


QVariant
Result::toVariant() const
{
    QVariantMap m;
    m.insert( "artist", artist()->name() );
    m.insert( "album", album()->name() );
    m.insert( "track", track() );
    m.insert( "source", friendlySource() );
    m.insert( "mimetype", mimetype() );
    m.insert( "size", size() );
    m.insert( "bitrate", bitrate() );
    m.insert( "duration", duration() );
    m.insert( "score", score() );
    m.insert( "sid", id() );
    m.insert( "discnumber", discnumber() );
    m.insert( "albumpos", albumpos() );

    if ( !composer().isNull() )
        m.insert( "composer", composer()->name() );

    return m;
}


QString
Result::toString() const
{
    return QString( "Result(%1, score: %2) %3 - %4%5 (%6)" )
              .arg( id() )
              .arg( score() )
              .arg( artist().isNull() ? QString() : artist()->name() )
              .arg( track() )
              .arg( album().isNull() || album()->name().isEmpty() ? "" : QString( " on %1" ).arg( album()->name() ) )
              .arg( url() );
}


Tomahawk::query_ptr
Result::toQuery()
{
    if ( m_query.isNull() )
    {
        m_query = Tomahawk::Query::get( artist()->name(), track(), album()->name() );

        if ( m_query.isNull() )
            return query_ptr();

        m_query->setAlbumPos( albumpos() );
        m_query->setDiscNumber( discnumber() );
        m_query->setDuration( duration() );
        if ( !composer().isNull() )
            m_query->setComposer( composer()->name() );

        QList<Tomahawk::result_ptr> rl;
        rl << Result::get( m_url );

        m_query->addResults( rl );
        m_query->setResolveFinished( true );
    }

    return m_query;
}


void
Result::updateAttributes()
{
    if ( m_attributes.contains( "releaseyear" ) )
    {
        m_year = m_attributes.value( "releaseyear" ).toInt();
    }
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
Result::setArtist( const Tomahawk::artist_ptr& artist )
{
    m_artist = artist;
}


void
Result::setComposer( const Tomahawk::artist_ptr &composer )
{
    m_composer = composer;
}


void
Result::setAlbum( const Tomahawk::album_ptr& album )
{
    m_album = album;
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
    m_resolvedBy = QWeakPointer< Tomahawk::Resolver >( resolver );
}


void
Result::doneEditing()
{
    m_query.clear();
    emit updated();
}
