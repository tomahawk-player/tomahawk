/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "EchonestCatalogSynchronizer.h"

#include <echonest/CatalogUpdateEntry.h>
#include <echonest/Config.h>

#include "collection/Collection.h"
#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "database/DatabaseCommand_GenericSelect.h"
#include "database/DatabaseCommand_SetCollectionAttributes.h"
#include "database/DatabaseCommand_LoadFiles.h"
#include "TomahawkSettings.h"
#include "SourceList.h"
#include "Query.h"
#include "utils/Logger.h"

#include <echonest/CatalogUpdateEntry.h>
#include <echonest/Config.h>

using namespace Tomahawk;

EchonestCatalogSynchronizer* EchonestCatalogSynchronizer::s_instance = 0;

EchonestCatalogSynchronizer::EchonestCatalogSynchronizer( QObject *parent )
    : QObject( parent )
{
    m_syncing = TomahawkSettings::instance()->enableEchonestCatalogs();

    qRegisterMetaType<QList<QStringList> >("QList<QStringList>");

    connect( TomahawkSettings::instance(), SIGNAL( changed() ), this, SLOT( checkSettingsChanged() ) );
    connect( SourceList::instance()->getLocal()->dbCollection().data(), SIGNAL( tracksAdded( QList<unsigned int> ) ), this, SLOT( tracksAdded( QList<unsigned int> ) ), Qt::QueuedConnection );
    connect( SourceList::instance()->getLocal()->dbCollection().data(), SIGNAL( tracksRemoved( QList<unsigned int> ) ), this, SLOT( tracksRemoved( QList<unsigned int> ) ), Qt::QueuedConnection );

    const QByteArray artist = TomahawkSettings::instance()->value( "collection/artistCatalog" ).toByteArray();
    const QByteArray song = TomahawkSettings::instance()->value( "collection/songCatalog" ).toByteArray();

    if ( !artist.isEmpty() )
        m_artistCatalog.setId( artist );
    if ( !song.isEmpty() )
        m_songCatalog.setId( song );

    // Sanity check
    if ( !song.isEmpty() && !m_syncing )
    {
        // Not syncing but have a catalog id... lets fix this
        QNetworkReply* r = m_songCatalog.deleteCatalog();
        connect( r, SIGNAL( finished() ), this, SLOT( catalogDeleted() ) );
        r->setProperty( "type", "song" );
    }
    if ( !artist.isEmpty() && !m_syncing )
    {
        QNetworkReply* r = m_artistCatalog.deleteCatalog();
        connect( r, SIGNAL( finished() ), this, SLOT( catalogDeleted() ) );
        r->setProperty( "type", "artist" );
    }
}

void
EchonestCatalogSynchronizer::checkSettingsChanged()
{
    if ( TomahawkSettings::instance()->enableEchonestCatalogs() && !m_syncing )
    {
        // enable, and upload whole db
        m_syncing = true;

        tDebug() << "Echonest Catalog sync pref changed, uploading!!";
        uploadDb();
    } else if ( !TomahawkSettings::instance()->enableEchonestCatalogs() && m_syncing )
    {

        tDebug() << "Found echonest change, doing catalog deletes!";
        // delete all track nums and catalog ids from our peers
        {
            DatabaseCommand_SetTrackAttributes* cmd = new DatabaseCommand_SetTrackAttributes( DatabaseCommand_SetTrackAttributes::EchonestCatalogId );
            Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );
        }
        {
            DatabaseCommand_SetCollectionAttributes* cmd = new DatabaseCommand_SetCollectionAttributes( DatabaseCommand_SetCollectionAttributes::EchonestSongCatalog, true );
            Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );
        }

        if ( !m_songCatalog.id().isEmpty() )
        {
            QNetworkReply* r = m_songCatalog.deleteCatalog();
            connect( r, SIGNAL( finished() ), this, SLOT( catalogDeleted() ) );
            r->setProperty( "type", "song" );
        }
        if ( !m_artistCatalog.id().isEmpty() )
        {
            QNetworkReply* r = m_artistCatalog.deleteCatalog();
            connect( r, SIGNAL( finished() ), this, SLOT( catalogDeleted() ) );
            r->setProperty( "type", "artist" );
        }
        m_syncing = false;
    }
}

void
EchonestCatalogSynchronizer::catalogDeleted()
{
    QNetworkReply* r = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( r );

    QString toDel = QString( "collection/%1Catalog" ).arg( r->property( "type" ).toString() );

    try
    {
        // HACK libechonest bug, should be a static method but it's not. Doesn't actually use any instance vars though
        m_songCatalog.parseDelete( r );
        // If we didn't throw, no errors, so clear our config
        TomahawkSettings::instance()->setValue( toDel, QString() );
    } catch ( const Echonest::ParseError& e )
    {
        tLog() << "Error in libechonest parsing catalog delete:" << e.what();
    }
}


void
EchonestCatalogSynchronizer::uploadDb()
{
    // create two catalogs: uuid_song, and uuid_artist.
    QNetworkReply* r =  Echonest::Catalog::create( QString( "%1_song" ).arg( Database::instance()->impl()->dbid() ), Echonest::CatalogTypes::Song );
    connect( r, SIGNAL( finished() ), this, SLOT( songCreateFinished() ) );

//     r =  Echonest::Catalog::create( QString( "%1_artist" ).arg( Database::instance()->dbid() ), Echonest::CatalogTypes::Artist );
//     connect( r, SIGNAL( finished() ), this, SLOT( artistCreateFinished() ) );
}


void
EchonestCatalogSynchronizer::songCreateFinished()
{
    QNetworkReply* r = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( r );

    tDebug() << "Finished creating song catalog, updating data now!!";
    try
    {
        m_songCatalog = Echonest::Catalog::parseCreate( r );
        TomahawkSettings::instance()->setValue( "collection/songCatalog", m_songCatalog.id() );
        QSharedPointer< DatabaseCommand > cmd( new DatabaseCommand_SetCollectionAttributes( DatabaseCommand_SetCollectionAttributes::EchonestSongCatalog,
                                                                                            m_songCatalog.id() ) );
        Database::instance()->enqueue( cmd );
    } catch ( const Echonest::ParseError& e )
    {
        tLog() << "Echonest threw an exception parsing song catalog create:" << e.what();
        return;
    }

    QString sql( "SELECT file.id, track.name, artist.name, album.name "
                 "FROM file, artist, track, file_join "
                 "LEFT OUTER JOIN album "
                 "ON file_join.album = album.id "
                 "WHERE file.id = file_join.file "
                 "AND file_join.artist = artist.id "
                 "AND file_join.track = track.id "
                 "AND file.source IS NULL");
    DatabaseCommand_GenericSelect* cmd = new DatabaseCommand_GenericSelect( sql, DatabaseCommand_GenericSelect::Track, true );
    connect( cmd, SIGNAL( rawData( QList< QStringList > ) ), this, SLOT( rawTracksAdd( QList< QStringList > ) ) );
    Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );
}


void
EchonestCatalogSynchronizer::artistCreateFinished()
{
    QNetworkReply* r = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( r );

    // We don't support artist catalogs at the moment
    return;
    /*
    try
    {
        m_artistCatalog = Echonest::Catalog::parseCreate( r );
        TomahawkSettings::instance()->setValue( "collection/artistCatalog", m_artistCatalog.id() );

//        QSharedPointer< DatabaseCommand > cmd( new DatabaseCommand_SetCollectionAttributes( SourceList::instance()->getLocal(),
//                                                                                            DatabaseCommand_SetCollectionAttributes::EchonestSongCatalog,
//                                                                                            m_songCatalog.id() ) );
//        Database::instance()->enqueue( cmd );
    } catch ( const Echonest::ParseError& e )
    {
        tLog() << "Echonest threw an exception parsing artist catalog create:" << e.what();
        return;
    }*/
}

void
EchonestCatalogSynchronizer::rawTracksAdd( const QList< QStringList >& tracks )
{
    tDebug() << "Got raw tracks, num:" << tracks.size();

//     int limit = ( tracks.size() < 1000 ) ? tracks.size() : 1000;

    int cur = 0;
    while ( cur < tracks.size() )
    {
        int prev = cur;
        cur = ( cur + 2000 > tracks.size() ) ? tracks.size() : cur + 2000;

        tDebug() << "Enqueueing a batch of tracks to upload to echonest catalog:" << cur - prev;
        Echonest::CatalogUpdateEntries entries;
        for ( int i = prev; i < cur; i++ )
        {
            if ( tracks[i][1].isEmpty() || tracks[i][2].isEmpty() )
                continue;
            entries.append( entryFromTrack( tracks[i], Echonest::CatalogTypes::Update ) );
        }
        tDebug() << "Done queuing:" << entries.size() << "tracks";
        m_queuedUpdates.enqueue( entries );
    }

    doUploadJob();

}

void
EchonestCatalogSynchronizer::doUploadJob()
{
    if ( m_queuedUpdates.isEmpty() )
        return;

    Echonest::CatalogUpdateEntries entries = m_queuedUpdates.dequeue();
    tDebug() << "Updating number of entries:" << entries.count();

    QNetworkReply* updateJob = m_songCatalog.update( entries );
    connect( updateJob, SIGNAL( finished() ), this, SLOT( songUpdateFinished() ) );
}


Echonest::CatalogUpdateEntry
EchonestCatalogSynchronizer::entryFromTrack( const QStringList& track, Echonest::CatalogTypes::Action action ) const
{
    //qDebug() << "UPLOADING:" << track[0] << track[1] << track[2];
    Echonest::CatalogUpdateEntry entry;
    entry.setAction( action );
    entry.setItemId(track[ 0 ].toLatin1() ); // track dbid
    entry.setSongName( escape( track[ 1 ] ) );
    entry.setArtistName( escape( track[ 2 ] ) );
    entry.setRelease( escape( track[ 3 ] ) );

    return entry;
}


void
EchonestCatalogSynchronizer::songUpdateFinished()
{
    QNetworkReply* r = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( r );

    try
    {
        QByteArray ticket = m_songCatalog.parseTicket( r );
        QNetworkReply* tJob = m_songCatalog.status( ticket );
        connect( tJob, SIGNAL( finished() ), this, SLOT( checkTicket() ) );
    } catch ( const Echonest::ParseError& e )
    {
        tLog() << "Echonest threw an exception parsing catalog update finished:" << e.what();
    }

    doUploadJob();
}

void
EchonestCatalogSynchronizer::checkTicket()
{
    QNetworkReply* r = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( r );

    try
    {
        Echonest::CatalogStatus status = m_songCatalog.parseStatus( r );

        tLog() << "Catalog status update:" << status.status << status.details << status.items;
    } catch ( const Echonest::ParseError& e )
    {
        tLog() << "Echonest threw an exception parsing catalog create:" << e.what();
        return;
    }
}

void
EchonestCatalogSynchronizer::tracksAdded( const QList< unsigned int >& tracks )
{
    if ( !m_syncing || m_songCatalog.id().isEmpty() || tracks.isEmpty() )
        return;

    qDebug() << Q_FUNC_INFO << "Got tracks added from db, fetching metadata" << tracks;
    // Get the result_ptrs from the tracks
    DatabaseCommand_LoadFiles* cmd = new DatabaseCommand_LoadFiles( tracks );
    connect( cmd, SIGNAL( results( QList<Tomahawk::result_ptr> ) ), this, SLOT( loadedResults( QList<Tomahawk::result_ptr> ) ) );
    Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );
}


void
EchonestCatalogSynchronizer::loadedResults( const QList<result_ptr>& results )
{
    QList< QStringList > rawTracks;
    qDebug() << Q_FUNC_INFO << "Got track metadata..." << results.size();

    foreach( const result_ptr& result, results )
    {
        if ( result.isNull() )
            continue;

        qDebug() << "Metadata for item:" << result->fileId();

        const QString artist = result->artist().isNull() ? QString() : result->artist()->name();
        const QString album = result->album().isNull() ? QString() : result->album()->name();
        rawTracks << ( QStringList() << QString::number( result->fileId() ) << result->track() << artist << album );
    }
    rawTracksAdd( rawTracks );
}

void
EchonestCatalogSynchronizer::tracksRemoved( const QList< unsigned int >& trackIds )
{

    if ( !m_syncing || m_songCatalog.id().isEmpty() || trackIds.isEmpty() )
        return;


    Echonest::CatalogUpdateEntries entries;
    entries.reserve( trackIds.size() );

    foreach ( unsigned int id, trackIds )
    {
        Echonest::CatalogUpdateEntry e( Echonest::CatalogTypes::Delete );
        e.setItemId( QString::number( id ).toLatin1() );
        entries.append( e );
    }

    QNetworkReply* reply = m_songCatalog.update( entries );
    connect( reply, SIGNAL( finished() ), this, SLOT( songUpdateFinished() ) );
}

QByteArray
EchonestCatalogSynchronizer::escape( const QString &in ) const
{
    // TODO echonest chokes on some chars in the output. But if we percent-encode those chars it works
    // We can't percent-encode the whole string, because then any UTF-8 chars that have been url-encoded, fail.
    // God this sucks. It's going to break...
    QString clean = in;
    clean.replace( "&", "%25" );
    clean.replace( ";", "%3B" );
    return clean.toUtf8();
    //return QUrl::toPercentEncoding( in. );
}
