/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "PlaylistEntry.h"

#include "Source.h"
#include "Query.h"
#include "Result.h"
#include "utils/Logger.h"

using namespace Tomahawk;


PlaylistEntry::PlaylistEntry()
{
}


PlaylistEntry::~PlaylistEntry()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << m_query->toString();
}


void
PlaylistEntry::setQueryVariant( const QVariant& v )
{
    QVariantMap m = v.toMap();

    QString artist = m.value( "artist" ).toString();
    QString album = m.value( "album" ).toString();
    QString track = m.value( "track" ).toString();

    setQuery( Tomahawk::Query::get( artist, track, album ) );
}


QVariant
PlaylistEntry::queryVariant() const
{
    return m_query->toVariant();
}


void
PlaylistEntry::setQuery( const Tomahawk::query_ptr& q )
{
    m_query = q;

    connect( q.data(), SIGNAL( resolvingFinished( bool ) ), SLOT( onQueryResolved( bool ) ) );
}


const Tomahawk::query_ptr&
PlaylistEntry::query() const
{
    return m_query;
}


source_ptr
PlaylistEntry::lastSource() const
{
    return m_lastsource;
}


void
PlaylistEntry::setLastSource( source_ptr s )
{
    m_lastsource = s;
}


void
PlaylistEntry::onQueryResolved( bool hasResults )
{
    if ( !hasResults )
        return;

    if ( resultHint().isEmpty() )
        setResultHint( hintFromQuery() );
}


void
PlaylistEntry::setResultHint( const QString& s )
{
    if ( m_resulthint == s )
        return;

    m_resulthint = s;
    emit resultChanged();
}


QString
PlaylistEntry::hintFromQuery() const
{
    QString resultHint, foundResult;
    if ( !m_query->results().isEmpty() )
        foundResult = m_query->results().first()->url();
    else if ( !m_query->resultHint().isEmpty() )
        foundResult = m_query->resultHint();

    if ( foundResult.startsWith( "file://" ) ||
        foundResult.startsWith( "servent://" ) || // Save resulthints for local files and peers automatically
        ( TomahawkUtils::whitelistedHttpResultHint( foundResult ) && m_query->saveHTTPResultHint() ) )
    {
        resultHint = foundResult;
    }

    return resultHint;
}
