/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#include "XspfGenerator.h"

#include "utils/Logger.h"

#include "Playlist.h"
#include "PlaylistEntry.h"
#include "Query.h"
#include "Source.h"
#include "Track.h"

#include <QXmlStreamWriter>
#include <QDateTime>
#include <QTimer>

using namespace Tomahawk;


XSPFGenerator::XSPFGenerator( const playlist_ptr& pl, QObject* parent )
    : QObject( parent )
    , m_playlist( pl )
{
     QTimer::singleShot( 0, this, SLOT( generate() ) );
}


XSPFGenerator::~XSPFGenerator()
{
}


void
XSPFGenerator::generate()
{
    Q_ASSERT( !m_playlist.isNull() );

    QByteArray xspf;
    QXmlStreamWriter w( &xspf );
    w.setAutoFormatting( true );
    w.writeStartDocument();

    w.writeStartElement( "playlist" );
    w.writeAttribute( "version", "1" );
    w.writeAttribute( "xmlns", "http://xspf.org/ns/0/" );

    w.writeTextElement( "title", m_playlist->title() );
    w.writeTextElement( "creator", m_playlist->creator() );
    w.writeTextElement( "date", QDateTime::fromTime_t( m_playlist->createdOn() ).toString( Qt::ISODate ) );

    w.writeStartElement( "trackList" );
    foreach ( const plentry_ptr& q, m_playlist->entries() )
    {
        w.writeStartElement( "track" );
        w.writeTextElement( "title", q->query()->queryTrack()->track() );
        w.writeTextElement( "creator", q->query()->queryTrack()->artist() );
        w.writeTextElement( "album", q->query()->queryTrack()->album() );
        w.writeEndElement();
    }
    w.writeEndDocument(); // will close all open elements

    emit generated( xspf );
}
