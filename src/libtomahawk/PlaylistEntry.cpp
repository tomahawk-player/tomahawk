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

#include "PlaylistEntry_p.h"

#include "utils/Logger.h"

#include "Result.h"
#include "Source.h"

using namespace Tomahawk;


PlaylistEntry::PlaylistEntry()
    : d_ptr( new PlaylistEntryPrivate( this ) )
{
}


PlaylistEntry::~PlaylistEntry()
{
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
    Q_D( const PlaylistEntry );
    return d->query->toVariant();
}


void
PlaylistEntry::setQuery( const Tomahawk::query_ptr& q )
{
    Q_D( PlaylistEntry );
    d->query = q;

    connect( q.data(), SIGNAL( resolvingFinished( bool ) ), SLOT( onQueryResolved( bool ) ) );
}


const Tomahawk::query_ptr&
PlaylistEntry::query() const
{
    Q_D( const PlaylistEntry );
    return d->query;
}


source_ptr
PlaylistEntry::lastSource() const
{
    Q_D( const PlaylistEntry );
    return d->lastsource;
}


void
PlaylistEntry::setLastSource( source_ptr s )
{
    Q_D( PlaylistEntry );
    d->lastsource = s;
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
    Q_D( PlaylistEntry );
    if ( d->resulthint == s )
        return;

    d->resulthint = s;
    emit resultChanged();
}


QString
PlaylistEntry::hintFromQuery() const
{
    Q_D( const PlaylistEntry );

    QString resultHint, foundResult;
    if ( !d->query->results().isEmpty() )
        foundResult = d->query->results().first()->url();
    else if ( !d->query->resultHint().isEmpty() )
        foundResult = d->query->resultHint();

    if ( foundResult.startsWith( "file://" ) ||
        foundResult.startsWith( "servent://" ) || // Save resulthints for local files and peers automatically
        ( TomahawkUtils::whitelistedHttpResultHint( foundResult ) && d->query->saveHTTPResultHint() ) )
    {
        resultHint = foundResult;
    }

    return resultHint;
}

bool
PlaylistEntry::isValid() const
{
    Q_D( const PlaylistEntry );

    return !d->query.isNull();
}


QString
PlaylistEntry::guid() const
{
    Q_D( const PlaylistEntry );
    return d->guid;
}


void
PlaylistEntry::setGuid( const QString& s )
{
    Q_D( PlaylistEntry );
    d->guid = s;
}


QString
PlaylistEntry::annotation() const
{
    Q_D( const PlaylistEntry );
    return d->annotation;
}


void
PlaylistEntry::setAnnotation( const QString& s )
{
    Q_D( PlaylistEntry );
    d->annotation = s;
}


QString
PlaylistEntry::resultHint() const
{
    Q_D( const PlaylistEntry );
    return d->resulthint;
}


unsigned int
PlaylistEntry::duration() const
{
    Q_D( const PlaylistEntry );
    return d->duration;
}


void
PlaylistEntry::setDuration( unsigned int i )
{
    Q_D( PlaylistEntry );
    d->duration = i;
}


unsigned int
PlaylistEntry::lastmodified() const
{
    Q_D( const PlaylistEntry );
    return d->lastmodified;
}

void
PlaylistEntry::setLastmodified( unsigned int i )
{
    Q_D( PlaylistEntry );
    d->lastmodified = i;
}
