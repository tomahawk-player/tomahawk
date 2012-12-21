/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
 *   Copyright 2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "M3uLoader.h"
#include "utils/Logger.h"
#include "utils/TomahawkUtils.h"
#include "Query.h"
#include "SourceList.h"
#include "Playlist.h"
#include "DropJob.h"
#include "ViewManager.h"
#include <QFileInfo>
#include <QFile>

/* taglib */
#include <taglib/fileref.h>
#include <taglib/tag.h>

using namespace Tomahawk;


M3uLoader::M3uLoader( const QStringList& urls, bool createNewPlaylist, QObject* parent )
    : QObject( parent )
    , m_single( false )
    , m_trackMode( true )
    , m_createNewPlaylist( createNewPlaylist )
    , m_urls( urls )
{
}


M3uLoader::M3uLoader( const QString& url, bool createNewPlaylist, QObject* parent )
    : QObject( parent )
    , m_single( false )
    , m_trackMode( true )
    , m_createNewPlaylist( createNewPlaylist )
    , m_urls( url )
{
}


M3uLoader::~M3uLoader()
{
}


void
M3uLoader::parse()
{
    foreach ( const QString& url, m_urls )
        parseM3u( url );
}


void
M3uLoader::getTags( const QFileInfo& info )
{
    QByteArray fileName = QFile::encodeName( info.canonicalFilePath() );
    const char *encodedName = fileName.constData();

    TagLib::FileRef f( encodedName );
    if( f.isNull() )
        return;
    TagLib::Tag *tag = f.tag();
    if( !tag )
        return;
    QString artist = TStringToQString( tag->artist() ).trimmed();
    QString album  = TStringToQString( tag->album() ).trimmed();
    QString track  = TStringToQString( tag->title() ).trimmed();

    if ( artist.isEmpty() || track.isEmpty() )
    {
        qDebug() << "Error parsing" << info.fileName();
        return;
    }
    else
    {
        qDebug() << Q_FUNC_INFO << artist << track << album;
        Tomahawk::query_ptr q = Tomahawk::Query::get( artist, track, album, uuid(), !m_createNewPlaylist );
        if ( !q.isNull() )
        {
            q->setResultHint( "file://" + info.absoluteFilePath() );
            q->setSaveHTTPResultHint( true );
            qDebug() << "Adding resulthint" << q->resultHint();
            m_tracks << q;
        }
    }
}

void
M3uLoader::parseLine( const QString& line, const QFile& file )
{
    QFileInfo tmpFile( QUrl::fromUserInput( QString( line.simplified() ) ).toLocalFile() );

    if( tmpFile.exists() )
    {
        getTags( tmpFile );
    }
    else
    {
        QUrl fileUrl = QUrl::fromUserInput( QString( QFileInfo(file).canonicalPath() + "/" + line.simplified() ) );
        QFileInfo tmpFile( fileUrl.toLocalFile() );
        if ( tmpFile.exists() )
        {
            getTags( tmpFile );
        }
    }
}

void
M3uLoader::parseM3u( const QString& fileLink )
{
    QFileInfo fileInfo( fileLink );
    QFile file( QUrl::fromUserInput( fileLink ).toLocalFile() );

    if ( !file.open( QIODevice::ReadOnly ) )
    {
        tDebug() << "Error opening m3u:" << file.errorString();
        return;
    }

    m_title = fileInfo.baseName();

    QTextStream stream( &file );
    QString singleLine;

    while ( !stream.atEnd() )
    {
        QString line = stream.readLine().trimmed();

        /// Fallback solution for, (drums) itunes!
        singleLine.append(line);

        /// If anyone wants to take on the regex for parsing EXT, go ahead
        /// But the notion that users does not tag by a common rule. that seems hard
        /// So ignore that for now
        if ( line.contains( "EXT" ) )
            continue;

        parseLine( line, file );

    }

    if ( m_tracks.isEmpty() )
    {
        if ( !singleLine.isEmpty() )
        {
            QStringList m3uList = singleLine.split("\r");
            foreach( const QString& line, m3uList )
                parseLine( line, file );
        }

        if ( m_tracks.isEmpty() )
        {
            tDebug() << "Could not parse M3U!";
            return;
        }
    }

    if ( m_createNewPlaylist )
    {
        m_playlist = Playlist::create( SourceList::instance()->getLocal(),
                                        uuid(),
                                        m_title,
                                        m_info,
                                        m_creator,
                                        false,
                                        m_tracks );

        connect( m_playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( playlistCreated() ) );
    }
    else
        emit tracks( m_tracks );
    m_tracks.clear();
}

void
M3uLoader::playlistCreated()
{
    ViewManager::instance()->show( m_playlist );
    deleteLater();
}

