/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
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

#include "m3uloader.h"
#include "utils/logger.h"
#include "utils/tomahawkutils.h"
#include "query.h"
#include "sourcelist.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "viewmanager.h"
#include <QtCore/QRegExp>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
/* taglib */
#include <taglib/fileref.h>
#include <taglib/tag.h>

using namespace Tomahawk;

M3uLoader::M3uLoader( const QStringList& Urls, bool createNewPlaylist, QObject* parent )
    : QObject ( parent )
    , m_single( false )
    , m_trackMode( true )
    , m_createNewPlaylist( createNewPlaylist )

{
    foreach ( const QString& url, Urls )
        parseM3u( url );
}

M3uLoader::M3uLoader( const QString& Url, bool createNewPlaylist, QObject* parent )
    : QObject ( parent )
    , m_single( false )
    , m_trackMode( true )
    , m_createNewPlaylist( createNewPlaylist )
{
    parseM3u( Url );
}

M3uLoader::~M3uLoader()
{
}

Tomahawk::query_ptr
M3uLoader::getTags( const QFileInfo& info )
{

    QByteArray fileName = QFile::encodeName( info.canonicalFilePath() );
    const char *encodedName = fileName.constData();

    TagLib::FileRef f( encodedName );
    TagLib::Tag *tag = f.tag();

    QString artist = TStringToQString( tag->artist() ).trimmed();
    QString album  = TStringToQString( tag->album() ).trimmed();
    QString track  = TStringToQString( tag->title() ).trimmed();

    if ( artist.isEmpty() || track.isEmpty() )
    {
        qDebug() << "Error parsing" << info.fileName();
        return Tomahawk::query_ptr();
    }
    else
    {
        qDebug() << Q_FUNC_INFO << artist << track << album;
        Tomahawk::query_ptr q = Tomahawk::Query::get( artist, track, album, uuid(), true );
        return q;
    }

}

void
M3uLoader::parseM3u( const QString& fileLink )
{
    QFileInfo fileInfo( fileLink );
    QFile file( QUrl::fromUserInput( fileLink ).toLocalFile() );

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug() << "Error" << file.errorString();
        return;
    }

    while (!file.atEnd()) {

         QByteArray line = file.readLine();
         /// If anyone wants to take on the regex for parsing EXT, go ahead
         /// But the notion that users does not tag by a common rule. that seems hard
         if( line.contains("EXT") )
             continue;

         qDebug() << line.simplified();
         QFileInfo tmpFile( QUrl::fromUserInput( QString( line.simplified() ) ).toLocalFile() );

         if( tmpFile.exists() )
             m_tracks << getTags( tmpFile );
         else
         {
             QFileInfo tmpFile( QUrl::fromUserInput( QString( fileInfo.canonicalPath() + "/" + line.simplified() ) ).toLocalFile() );
             if( tmpFile.exists() )
                 m_tracks << getTags( tmpFile );
         }

    }


    if( !m_tracks.isEmpty() ){
        qDebug() << Q_FUNC_INFO << "Emitting tracks!";
        emit tracks( m_tracks );
    }

    file.deleteLater();
}

