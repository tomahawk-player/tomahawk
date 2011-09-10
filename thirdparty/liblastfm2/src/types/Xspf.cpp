/*
   Copyright 2009 Last.fm Ltd. 
      - Primarily authored by Max Howell, Jono Cole and Doug Mansell

   This file is part of liblastfm.

   liblastfm is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   liblastfm is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with liblastfm.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QTimer>
#include <QUrl>

#include "../core/XmlQuery.h"

#include "Xspf.h"


lastfm::Xspf::Xspf( const QDomElement& playlist_node, QObject* parent )
    :QObject( parent )
{
    XmlQuery e( playlist_node );

    int expirySeconds = e["link rel=http://www.last.fm/expiry"].text().toInt();
    QTimer::singleShot( expirySeconds * 1000, this, SLOT(onExpired()));
    
    m_title = e["title"].text();
        
    //FIXME should we use UnicornUtils::urlDecode()?
    //The title is url encoded, has + instead of space characters 
    //and has a + at the begining. So it needs cleaning up:
    m_title.replace( '+', ' ' );
    m_title = QUrl::fromPercentEncoding( m_title.toAscii());
    m_title = m_title.trimmed();
    
    foreach (XmlQuery e, e["trackList"].children( "track" ))
    {
        MutableTrack t;
        t.setUrl( e["location"].text() );
        t.setExtra( "trackauth", e["extension"]["trackauth"].text() );
        t.setTitle( e["title"].text() );
        t.setArtist( e["creator"].text() );
        t.setAlbum( e["album"].text() );
        t.setDuration( e["duration"].text().toInt() / 1000 );
        t.setLoved( e["extension"]["loved"].text() == "1" );
        t.setSource( Track::LastFmRadio );

        QList<QString> contexts;
        QDomNodeList contextsNodeList = QDomElement(e["extension"]["context"]).childNodes();

        for ( int i = 0 ; i < contextsNodeList.count() ; ++i )
            contexts.append( contextsNodeList.item(i).toElement().text() );

        if ( contexts.count() > 0 )
            t.setContext( TrackContext( contextsNodeList.item(0).toElement().tagName(), contexts ) );

        m_tracks << t; // outside try block since location is enough basically
    }
}

void
lastfm::Xspf::onExpired()
{
    emit expired();
}
