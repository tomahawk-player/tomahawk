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

#include "xspfloader.h"

#include <QApplication>
#include <QDomDocument>
#include <QMessageBox>

#include "utils/tomahawkutils.h"
#include "utils/logger.h"

#include "sourcelist.h"
#include "playlist.h"

using namespace Tomahawk;


void
XSPFLoader::setOverrideTitle( const QString& newTitle )
{
    m_overrideTitle = newTitle;
}


void
XSPFLoader::load( const QUrl& url )
{
    QNetworkRequest request( url );
    Q_ASSERT( TomahawkUtils::nam() != 0 );
    QNetworkReply* reply = TomahawkUtils::nam()->get( request );

    // isn't there a race condition here? something could happen before we connect()
    connect( reply, SIGNAL( finished() ),
                      SLOT( networkLoadFinished() ) );

    connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
                      SLOT( networkError( QNetworkReply::NetworkError ) ) );
}


void
XSPFLoader::load( QFile& file )
{
    if( file.open( QFile::ReadOnly ) )
    {
        m_body = file.readAll();
        gotBody();
    }
    else
    {
        qDebug() << "Failed to open xspf file";
        reportError();
    }
}


void
XSPFLoader::reportError()
{
    qDebug() << Q_FUNC_INFO;
    emit failed();
    deleteLater();
}


void
XSPFLoader::networkLoadFinished()
{
    qDebug() << Q_FUNC_INFO;
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    m_body = reply->readAll();
    gotBody();
}


void
XSPFLoader::networkError( QNetworkReply::NetworkError e )
{
    qDebug() << Q_FUNC_INFO << e;
    reportError();
}


void
XSPFLoader::gotBody()
{
    qDebug() << Q_FUNC_INFO;

    QDomDocument xmldoc;
    bool namespaceProcessing = true;
    xmldoc.setContent( m_body, namespaceProcessing );
    QDomElement docElement( xmldoc.documentElement() );

    QString origTitle;
    QDomNodeList tracklist;
    QDomElement n = docElement.firstChildElement();
    for ( ; !n.isNull(); n = n.nextSiblingElement() ) {
        if (n.namespaceURI() == m_NS && n.localName() == "title") {
            origTitle = n.text();
        } else if (n.namespaceURI() == m_NS && n.localName() == "creator") {
            m_creator = n.text();
        } else if (n.namespaceURI() == m_NS && n.localName() == "info") {
            m_info = n.text();
        } else if (n.namespaceURI() == m_NS && n.localName() == "trackList") {
            tracklist = n.childNodes();
        }
    }

    m_title = origTitle;
    if ( m_title.isEmpty() )
        m_title = tr( "New Playlist" );
    if( !m_overrideTitle.isEmpty() )
        m_title = m_overrideTitle;

    bool shownError = false;
    for ( unsigned int i = 0; i < tracklist.length(); i++ )
    {
        QDomNode e = tracklist.at( i );

        QString artist, album, track, duration, annotation, url;
        QDomElement n = e.firstChildElement();
        for ( ; !n.isNull(); n = n.nextSiblingElement() )
        {
            if (n.namespaceURI() == m_NS && n.localName() == "duration") {
                duration = n.text();
            } else if (n.namespaceURI() == m_NS && n.localName() == "annotation") {
                annotation = n.text();
            } else if (n.namespaceURI() == m_NS && n.localName() == "creator") {
                artist = n.text();
            } else if (n.namespaceURI() == m_NS && n.localName() == "album") {
                album = n.text();
            } else if (n.namespaceURI() == m_NS && n.localName() == "title") {
                track = n.text();
            } else if (n.namespaceURI() == m_NS && n.localName() == "url") {
                url = n.text();
            }
        }

        if( artist.isEmpty() || track.isEmpty() )
        {
            if( !shownError )
            {
                QMessageBox::warning( 0, tr( "Failed to save tracks" ), tr( "Some tracks in the playlist do not contain an artist and a title. They will be ignored." ), QMessageBox::Ok );
                shownError = true;
            }
            continue;
        }

        query_ptr q = Tomahawk::Query::get( artist, track, album, uuid() );
        q->setDuration( duration.toInt() / 1000 );
        if( !url.isEmpty() )
            q->setResultHint( url );

        m_entries << q;
    }

    if ( origTitle.isEmpty() && m_entries.isEmpty() )
    {
        if ( m_autoCreate )
        {
            QMessageBox::critical( 0, tr( "XSPF Error" ), tr( "This is not a valid XSPF playlist." ) );
            deleteLater();
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

        deleteLater();
    }

    emit ok( m_playlist );
}
