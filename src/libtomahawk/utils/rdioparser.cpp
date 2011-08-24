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

#include "rdioparser.h"

#include <QUrl>
#include <QStringList>
#include "shortenedlinkparser.h"

using namespace Tomahawk;

RdioParser::RdioParser( QObject* parent )
    : QObject( parent )
    , m_count( 0 )
{
}

RdioParser::~RdioParser()
{
}

void
RdioParser::parse( const QString& url )
{
    m_multi = false;
    m_total = 1;
    parseUrl( url );
}

void
RdioParser::parse( const QStringList& urls )
{
    m_multi = true;
    m_total = urls.count();

    foreach( const QString& url, urls )
        parseUrl( url );
}


void
RdioParser::parseUrl( const QString& url )
{
    if ( url.contains( "rd.io" ) ) // shortened
    {
        ShortenedLinkParser* p = new ShortenedLinkParser( QStringList() << url, this );
        connect( p, SIGNAL( urls( QStringList ) ), this, SLOT( expandedLinks( QStringList ) ) );
        return;
    }

    query_ptr query;
    m_count++;

    if ( url.contains( "artist" ) && url.contains( "album" ) )
    {
        // this is a "full" url, no redirection needed
        QString realUrl = QUrl::fromUserInput( url ).toString().replace( "_", " " );

        QString artist, trk, album;
        QString matchStr = "/%1/([^/]*)/";
        QRegExp r( QString( matchStr ).arg( "artist" ) );

        int loc = r.indexIn( realUrl );
        if ( loc >= 0 )
            artist = r.cap( 1 );

        r = QRegExp( QString( matchStr ).arg( "album" ) );
        loc = r.indexIn( realUrl );
        if ( loc >= 0 )
            album = r.cap( 1 );

        r = QRegExp( QString( matchStr ).arg( "track" ) );
        loc = r.indexIn( realUrl );
        if ( loc >= 0 )
            trk = r.cap( 1 );

        if ( !trk.isEmpty() && !artist.isEmpty() )
        {
            query = Query::get( artist, trk, album, uuid(), true );
        }
    }

    if ( m_multi )
    {
        if ( !query.isNull() )
            m_queries << query;

        if ( m_count == m_total )
            emit tracks( m_queries );
    }
    if ( !m_multi && !query.isNull() )
        emit track( query );
}

void
RdioParser::expandedLinks( const QStringList& urls )
{
    foreach( const QString& url, urls )
    {
        if ( url.contains( "rdio.com" ) || url.contains( "rd.io" ) )
            parseUrl( url );
    }
}

