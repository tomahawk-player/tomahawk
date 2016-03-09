/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2015,      Dominik Schmidt <domme@tomahawk-player.org>
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

#include "collection/Collection.h"
#include "database/Database.h"
#include "filemetadata/MetadataEditor.h"
#include "resolvers/ExternalResolverGui.h"
#include "resolvers/Resolver.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include "Album.h"
#include "Pipeline.h"
#include "PlaylistInterface.h"
#include "Source.h"
#include "TomahawkSettings.h"
#include "Track.h"
#include "Typedefs.h"

#include <QCoreApplication>

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
Result::get( const QString& url, const track_ptr& track )
{
    if ( url.trimmed().isEmpty() || track.isNull() )
    {
        return result_ptr();
    }

    QMutexLocker lock( &s_mutex );
    if ( s_results.contains( url ) )
    {
        return s_results.value( url );
    }

    result_ptr r = result_ptr( new Result( url, track ), &Result::deleteLater );
    r->moveToThread( QCoreApplication::instance()->thread() );
    r->setWeakRef( r.toWeakRef() );
    s_results.insert( url, r );

    return r;
}


result_ptr
Result::getCached( const QString& url )
{
    if ( url.trimmed().isEmpty() )
    {
        return result_ptr();
    }

    QMutexLocker lock( &s_mutex );
    if ( s_results.contains( url ) )
    {
        return s_results.value( url );
    }

    return result_ptr();
}


Result::Result( const QString& url, const track_ptr& track )
    : QObject()
    , m_url( url )
    , m_isPreview( false )
    , m_checked( false )
    , m_bitrate( 0 )
    , m_size( 0 )
    , m_modtime( 0 )
    , m_fileId( 0 )
    , m_track( track )
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


void
Result::onResolverRemoved( Tomahawk::Resolver* resolver )
{
    m_mutex.lock();

    if ( m_resolver.data() == resolver )
    {
        m_resolver = 0;
        m_mutex.unlock();

        emit statusChanged();
    }
    else
    {
        m_mutex.unlock();
    }
}


collection_ptr
Result::resolvedByCollection() const
{
    return m_collection;
}


QString
Result::url() const
{
    QMutexLocker lock( &m_mutex );

    return m_url;
}


bool
Result::checked() const
{
    QMutexLocker lock( &m_mutex );

    return m_checked;
}


bool
Result::isPreview() const
{
    QMutexLocker lock( &m_mutex );

    return m_isPreview;
}


QString
Result::mimetype() const
{
    QMutexLocker lock( &m_mutex );

    return m_mimetype;
}


RID
Result::id() const
{
    QMutexLocker lock( &m_mutex );

    if ( m_rid.isEmpty() )
        m_rid = uuid();

    return m_rid;
}


bool
Result::isOnline() const
{
    if ( !resolvedByCollection().isNull() )
    {
        return resolvedByCollection()->isOnline();
    }
    else
    {
        QMutexLocker lock( &m_mutex );

        return !m_resolver.isNull();
    }
}


bool
Result::playable() const
{
    if ( resolvedByCollection() )
    {
        return resolvedByCollection()->isOnline();
    }

    return true;
}


bool
Result::isLocal() const
{
    return resolvedByCollection().isNull() ? false : resolvedByCollection()->isLocal();
}


QVariant
Result::toVariant() const
{
    track_ptr t = track();

    QVariantMap m;
    m.insert( "artist", t->artist() );
    m.insert( "album", t->album() );
    m.insert( "track", t->track() );
    m.insert( "source", friendlySource() );
    m.insert( "mimetype", mimetype() );
    m.insert( "size", size() );
    m.insert( "bitrate", bitrate() );
    m.insert( "duration", t->duration() );
//    m.insert( "score", score() );
    m.insert( "sid", id() );
    m.insert( "discnumber", t->discnumber() );
    m.insert( "albumpos", t->albumpos() );
    m.insert( "preview", isPreview() );
    m.insert( "purchaseUrl", purchaseUrl() );

    if ( !t->composer().isEmpty() )
        m.insert( "composer", t->composer() );

    return m;
}


QString
Result::toString() const
{
    m_mutex.lock();
    track_ptr track = m_track;
    QString url = m_url;
    m_mutex.unlock();

    if ( track )
    {
        return QString( "Result(%1) %2 - %3%4 (%5)" )
                  .arg( id() )
                  .arg( track->artist() )
                  .arg( track->track() )
                  .arg( track->album().isEmpty() ? QString() : QString( " on %1" ).arg( track->album() ) )
                  .arg( url );
    }
    else
    {
        return QString( "Result(%1) (%2)" )
                  .arg( id() )
                  .arg( url );
    }
}


Tomahawk::query_ptr
Result::toQuery()
{
    QMutexLocker l( &m_mutex );

    if ( m_query.isNull() )
    {
        query_ptr query = Tomahawk::Query::get( m_track );
        if ( !query )
            return query_ptr();

        m_query = query->weakRef();

        QList<Tomahawk::result_ptr> rl;
        rl << m_ownRef.toStrongRef();
        m_mutex.unlock();
        query->addResults( rl );
        m_mutex.lock();
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
Result::setResolvedByCollection( const Tomahawk::collection_ptr& collection, bool emitOnlineEvents )
{
    m_collection = collection;

    if ( emitOnlineEvents )
    {
        Q_ASSERT( !collection.isNull() );
        connect( collection.data(), SIGNAL( destroyed( QObject * ) ), SLOT( onOffline() ), Qt::QueuedConnection );
        connect( collection.data(), SIGNAL( online() ), SLOT( onOnline() ), Qt::QueuedConnection );
        connect( collection.data(), SIGNAL( offline() ), SLOT( onOffline() ), Qt::QueuedConnection );
    }
}


void
Result::setFriendlySource( const QString& s )
{
    QMutexLocker lock( &m_mutex );

    m_friendlySource = s;
}


void
Result::setPreview( bool isPreview )
{
    QMutexLocker lock( &m_mutex );

    m_isPreview = isPreview;
}


void
Result::setPurchaseUrl( const QString& u )
{
    QMutexLocker lock( &m_mutex );

    m_purchaseUrl = u;
}


void
Result::setLinkUrl( const QString& u )
{
    QMutexLocker lock( &m_mutex );

    m_linkUrl = u;
}


void
Result::setChecked( bool checked )
{
    QMutexLocker lock( &m_mutex );

    m_checked = checked;
}


void
Result::setMimetype( const QString& mimetype )
{
    QMutexLocker lock( &m_mutex );

    m_mimetype = mimetype;
}


void
Result::setBitrate( unsigned int bitrate )
{
    QMutexLocker lock( &m_mutex );

    m_bitrate = bitrate;
}


void
Result::setSize( unsigned int size )
{
    QMutexLocker lock( &m_mutex );

    m_size = size;
}


void
Result::setModificationTime( unsigned int modtime )
{
    QMutexLocker lock( &m_mutex );

    m_modtime = modtime;
}


void
Result::setTrack( const track_ptr& track )
{
    QMutexLocker lock( &m_mutex );

    m_track = track;
}


unsigned int
Result::fileId() const
{
    QMutexLocker lock( &m_mutex );

    return m_fileId;
}


QString
Result::friendlySource() const
{
    if ( resolvedByCollection().isNull() )
    {
        QMutexLocker lock( &m_mutex );

        return m_friendlySource;
    }
    else
        return resolvedByCollection()->prettyName();
}


QString
Result::purchaseUrl() const
{
    QMutexLocker lock( &m_mutex );

    return m_purchaseUrl;
}


QString
Result::linkUrl() const
{
    QMutexLocker lock( &m_mutex );

    return m_linkUrl;
}


QPixmap
Result::sourceIcon( TomahawkUtils::ImageMode style, const QSize& desiredSize ) const
{
    if ( resolvedByCollection().isNull() )
    {
        //QMutexLocker lock( &m_mutex );

        const ExternalResolver* resolver = qobject_cast< ExternalResolver* >( m_resolver.data() );
        if ( !resolver )
        {
            return QPixmap();
        }
        else
        {
            QMutexLocker l( &s_sourceIconMutex );

            const QString key = sourceCacheKey( m_resolver.data(), desiredSize, style );
            if ( !sourceIconCache()->contains( key ) )
            {
                QPixmap pixmap = resolver->icon( desiredSize );
                if ( pixmap.isNull() )
                    return pixmap;

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
        QPixmap avatar = resolvedByCollection()->icon( desiredSize );
        return avatar;
    }
}


unsigned int
Result::bitrate() const
{
    QMutexLocker lock( &m_mutex );

    return m_bitrate;
}


unsigned int
Result::size() const
{
    QMutexLocker lock( &m_mutex );

    return m_size;
}


unsigned int
Result::modificationTime() const
{
    QMutexLocker lock( &m_mutex );

    return m_modtime;
}


void
Result::setFileId( unsigned int id )
{
    QMutexLocker lock( &m_mutex );

    m_fileId = id;
}


Tomahawk::Resolver*
Result::resolvedBy() const
{
    QMutexLocker lock( &m_mutex );

    if ( !m_collection.isNull() )
        return m_collection.data();

    return m_resolver.data();
}


void
Result::setResolvedByResolver( Tomahawk::Resolver* resolver )
{
    QMutexLocker lock( &m_mutex );

    m_resolver = QPointer< Tomahawk::Resolver >( resolver );
}


QPointer< Resolver > Result::resolvedByResolver() const
{
    QMutexLocker lock( &m_mutex );

    return m_resolver;
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
    QMutexLocker lock( &m_mutex );

    return m_track;
}


QList< DownloadFormat >
Result::downloadFormats() const
{
    QMutexLocker lock( &m_mutex );

    return m_formats;
}


void
Result::setDownloadFormats( const QList<DownloadFormat>& formats )
{
    if ( formats.isEmpty() )
        return;

    QMutexLocker lock( &m_mutex );

    m_formats.clear();
    foreach ( const DownloadFormat& format, formats )
    {
        if ( format.extension.toLower() == TomahawkSettings::instance()->downloadsPreferredFormat().toLower() )
        {
            m_formats.insert( 0, format );
        }
        else
        {
            m_formats << format;
        }
    }

    if ( !m_formats.isEmpty() )
    {
        connect( TomahawkSettings::instance(), SIGNAL( changed() ), this, SLOT( onSettingsChanged() ), Qt::UniqueConnection );
    }
    else
    {
        disconnect( TomahawkSettings::instance(), SIGNAL( changed() ), this, SLOT( onSettingsChanged() ) );
    }
}


void
Result::onSettingsChanged()
{
    if ( TomahawkSettings::instance()->downloadsPreferredFormat().toLower() != downloadFormats().first().extension.toLower() )
    {
        setDownloadFormats( downloadFormats() );
        emit updated();
    }
}


downloadjob_ptr
Result::toDownloadJob( const DownloadFormat& format )
{
    if ( !m_downloadJob )
    {
        m_downloadJob = downloadjob_ptr( new DownloadJob( weakRef().toStrongRef(), format ) );
        connect( m_downloadJob.data(), SIGNAL( progress( int ) ), SIGNAL( updated() ) );
        connect( m_downloadJob.data(), SIGNAL( stateChanged( DownloadJob::TrackState, DownloadJob::TrackState ) ),
                                         SLOT( onDownloadJobStateChanged( DownloadJob::TrackState, DownloadJob::TrackState ) ) );
    }

    return m_downloadJob;
}


void
Result::onDownloadJobStateChanged( DownloadJob::TrackState newState, DownloadJob::TrackState oldState )
{
    if ( newState == DownloadJob::Aborted )
    {
        m_downloadJob.clear();
        emit updated();
    }
}


QWeakPointer<Result>
Result::weakRef()
{
    QMutexLocker lock( &m_mutex );

    return m_ownRef;
}


void
Result::setWeakRef( QWeakPointer<Result> weakRef )
{
    QMutexLocker lock( &m_mutex );

    m_ownRef = weakRef;
}
