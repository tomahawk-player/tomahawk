/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */


#include "JspfLoader.h"

#include <QApplication>
#include <QDomDocument>
#include <QMessageBox>

#include <qjson/parser.h>

#include "SourceList.h"
#include "Playlist.h"

#include "utils/NetworkReply.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

using namespace Tomahawk;


JSPFLoader::JSPFLoader( bool autoCreate, QObject* parent )
    : QObject( parent )
    , m_autoCreate( autoCreate )
    , m_autoDelete( true )
{
}


JSPFLoader::~JSPFLoader()
{
}


QList< Tomahawk::query_ptr >
JSPFLoader::entries() const
{
    return m_entries;
}


void
JSPFLoader::load( const QUrl& url )
{
    QNetworkRequest request( url );

    Q_ASSERT( TomahawkUtils::nam() != 0 );
    NetworkReply* reply = new NetworkReply( TomahawkUtils::nam()->get( request ) );

    connect( reply, SIGNAL( finished() ), SLOT( networkLoadFinished() ) );
    connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ), SLOT( networkError( QNetworkReply::NetworkError ) ) );
}


void
JSPFLoader::load( QFile& file )
{
    if ( file.open( QFile::ReadOnly ) )
    {
        m_body = file.readAll();
        gotBody();
    }
    else
    {
        tLog() << "Failed to open jspf file";
        reportError();
    }
}


void
JSPFLoader::reportError()
{
    emit failed();

    if ( m_autoDelete )
        deleteLater();
}


void
JSPFLoader::networkLoadFinished()
{
    NetworkReply* r = qobject_cast<NetworkReply*>( sender() );
    if ( r->reply()->error() == QNetworkReply::NoError )
    {
        m_body = r->reply()->readAll();
        gotBody();
    }

    r->deleteLater();
}


void
JSPFLoader::networkError( QNetworkReply::NetworkError e )
{
    Q_UNUSED( e );

    reportError();

    NetworkReply* r = qobject_cast<NetworkReply*>( sender() );
    r->deleteLater();
}


void
JSPFLoader::gotBody()
{
    QJson::Parser p;
    bool retOk;
    QVariantMap wrapper = p.parse( m_body, &retOk ).toMap();

    if ( !retOk )
    {
        tLog() << "Failed to parse jspf json:" << p.errorString() << "on line" << p.errorLine();
        return;
    }

    if ( !wrapper.contains( "playlist" ) )
    {
        tLog() << "No playlist element in JSPF!";
        return;
    }

    QVariantMap pl = wrapper.value( "playlist" ).toMap();
    QString origTitle = pl.value( "title" ).toString();
    m_info = pl.value( "info" ).toString();
    m_creator = pl.value( "creator" ).toString();

    m_title = origTitle;
    if ( m_title.isEmpty() )
        m_title = tr( "New Playlist" );
    if ( !m_overrideTitle.isEmpty() )
        m_title = m_overrideTitle;

    if ( pl.contains( "track" ) )
    {
        QVariantList tracks = pl.value( "track" ).toList();

        bool shownError = false;
        foreach ( const QVariant& currentTrack, tracks )
        {
            QVariantMap tM = currentTrack.toMap();
            QString artist, album, track, duration, annotation, url;

            artist = tM.value( "creator" ).toString();
            album = tM.value( "album" ).toString();
            track = tM.value( "title" ).toString();
            duration = tM.value( "duration" ).toString();
            annotation = tM.value( "annotation" ).toString();
            if ( tM.value( "location" ).toList().size() > 0 )
                url = tM.value( "location" ).toList().first().toString();

            if ( artist.isEmpty() || track.isEmpty() )
            {
                if ( !shownError )
                {
                    QMessageBox::warning( 0, tr( "Failed to save tracks" ), tr( "Some tracks in the playlist do not contain an artist and a title. They will be ignored." ), QMessageBox::Ok );
                    shownError = true;
                }
                continue;
            }

            query_ptr q = Tomahawk::Query::get( artist, track, album, uuid() );
            if ( q.isNull() )
                continue;

            q->setDuration( duration.toInt() / 1000 );
            if ( !url.isEmpty() )
            {
                q->setResultHint( url );
                q->setSaveHTTPResultHint( true );
            }

            m_entries << q;
        }
    }

    if ( origTitle.isEmpty() && m_entries.isEmpty() )
    {
        if ( m_autoCreate )
        {
            QMessageBox::critical( 0, tr( "XSPF Error" ), tr( "This is not a valid XSPF playlist." ) );
            return;
        }
        else
        {
            emit failed();
            return;
        }
    }

    if ( m_autoCreate )
    {
        m_playlist = Playlist::create( SourceList::instance()->getLocal(),
                                       uuid(),
                                       m_title,
                                       m_info,
                                       m_creator,
                                       false,
                                       m_entries );
    }

    emit ok( m_playlist );

    if ( m_autoDelete )
        deleteLater();
}
