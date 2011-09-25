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

#include "database/database.h"
#include "database/databasecommand_genericselect.h"
#include "database/databasecommand_setcollectionattributes.h"
#include "tomahawksettings.h"
#include "sourcelist.h"

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

    const QByteArray artist = TomahawkSettings::instance()->value( "collection/artistCatalog" ).toByteArray();
    const QByteArray song = TomahawkSettings::instance()->value( "collection/songCatalog" ).toByteArray();
    if ( !artist.isEmpty() )
        m_artistCatalog.setId( artist );
    if ( !song.isEmpty() )
        m_songCatalog.setId( song );
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
        m_songCatalog.deleteCatalog();
        m_artistCatalog.deleteCatalog();
        m_syncing = false;
    }
}

void
EchonestCatalogSynchronizer::uploadDb()
{
    // create two catalogs: uuid_song, and uuid_artist.
    QNetworkReply* r =  Echonest::Catalog::create( QString( "%1_song" ).arg( Database::instance()->dbid() ), Echonest::CatalogTypes::Song );
    connect( r, SIGNAL( finished() ), this, SLOT( songCreateFinished() ) );

    r =  Echonest::Catalog::create( QString( "%1_artist" ).arg( Database::instance()->dbid() ), Echonest::CatalogTypes::Artist );
    connect( r, SIGNAL( finished() ), this, SLOT( artistCreateFinished() ) );
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
        QSharedPointer< DatabaseCommand > cmd( new DatabaseCommand_SetCollectionAttributes( SourceList::instance()->getLocal(),
                                                                                                    DatabaseCommand_SetCollectionAttributes::EchonestSongCatalog,
                                                                                                    m_songCatalog.id() ) );
        Database::instance()->enqueue( cmd );
    } catch ( const Echonest::ParseError& e )
    {
        tLog() << "Echonest threw an exception parsing song catalog create:" << e.what();
        return;
    }

    QString sql( "SELECT track.name, artist.name, album.name "
                 "FROM file, artist, track, file_join "
                 "LEFT OUTER JOIN album "
                 "ON file_join.album = album.id "
                 "WHERE file.id = file_join.file "
                 "AND file_join.artist = artist.id "
                 "AND file_join.track = track.id "
                 "AND file.source IS NULL");
    DatabaseCommand_GenericSelect* cmd = new DatabaseCommand_GenericSelect( sql, DatabaseCommand_GenericSelect::Track, true );
    connect( cmd, SIGNAL( rawData( QList< QStringList > ) ), this, SLOT( rawTracks( QList< QStringList > ) ) );
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
EchonestCatalogSynchronizer::rawTracks( const QList< QStringList >& tracks )
{
    tDebug() << "Got raw tracks, num:" << tracks.size();

//     int limit = ( tracks.size() < 1000 ) ? tracks.size() : 1000;

    int cur = 0;
    while ( cur < tracks.size() )
    {
        int prev = cur;
        cur = ( cur + 2000 > tracks.size() ) ? tracks.size() : cur + 2000;

        tDebug() << "Enqueueing a batch of tracks to upload to echonest catalog:" << cur - prev;
        Echonest::CatalogUpdateEntries entries( cur - prev );
        for ( int i = prev; i < cur; i++ )
        {
            if ( tracks[i][0].isEmpty() || tracks[i][1].isEmpty() )
                continue;
            entries.append( entryFromTrack( tracks[i] ) );
        }
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

    QNetworkReply* updateJob = m_songCatalog.update( entries );
    connect( updateJob, SIGNAL( finished() ), this, SLOT( songUpdateFinished() ) );
}


Echonest::CatalogUpdateEntry
EchonestCatalogSynchronizer::entryFromTrack( const QStringList& track ) const
{
    //qDebug() << "UPLOADING:" << track[0] << track[1] << track[2];
    Echonest::CatalogUpdateEntry entry;
    entry.setAction( Echonest::CatalogTypes::Update );
    entry.setSongName( escape( track[ 0 ] ) );
    entry.setArtistName( escape( track[ 1 ] ) );
    entry.setRelease( escape( track[ 2 ] ) );
    entry.setItemId( uuid().toUtf8() );

    return entry;
}


void
EchonestCatalogSynchronizer::songUpdateFinished()
{
    QNetworkReply* r = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( r );

    doUploadJob();

    try
    {
        QByteArray ticket = m_songCatalog.parseTicket( r );
        QNetworkReply* tJob = m_songCatalog.status( ticket );
        connect( tJob, SIGNAL( finished() ), this, SLOT( checkTicket() ) );
    } catch ( const Echonest::ParseError& e )
    {
        tLog() << "Echonest threw an exception parsing catalog update finished:" << e.what();
        return;
    }
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

QByteArray
EchonestCatalogSynchronizer::escape( const QString &in ) const
{
    return QUrl::toPercentEncoding( in );
}
