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

#include "XspfUpdater.h"

#include "playlist.h"
#include "utils/xspfloader.h"
#include "tomahawksettings.h"
#include "pipeline.h"
#include "utils/tomahawkutils.h"

#include <QTimer>

using namespace Tomahawk;

XspfUpdater::XspfUpdater( const playlist_ptr& pl, const QString& xUrl )
    : PlaylistUpdaterInterface( pl )
    , m_url( xUrl )
{
}

XspfUpdater::XspfUpdater( const playlist_ptr& pl, int interval, bool autoUpdate, const QString& xspfUrl )
    : PlaylistUpdaterInterface( pl, interval, autoUpdate )
    , m_url( xspfUrl )
{

}


XspfUpdater::XspfUpdater( const playlist_ptr& pl )
    : PlaylistUpdaterInterface( pl )
{

}


XspfUpdater::~XspfUpdater()
{}

void
XspfUpdater::updateNow()
{
    XSPFLoader* l = new XSPFLoader( false, false );
    l->setAutoResolveTracks( false );
    l->load( m_url );
    connect( l, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( playlistLoaded( QList<Tomahawk::query_ptr> ) ) );
}

void
XspfUpdater::playlistLoaded( const QList<Tomahawk::query_ptr>& newEntries )
{
    QList< query_ptr > tracks;
    foreach ( const plentry_ptr ple, playlist()->entries() )
        tracks << ple->query();

    bool changed = false;
    QList< query_ptr > mergedTracks = TomahawkUtils::mergePlaylistChanges( tracks, newEntries, changed );

    if ( !changed )
        return;

    QList<Tomahawk::plentry_ptr> el = playlist()->entriesFromQueries( mergedTracks, true );
    playlist()->createNewRevision( uuid(), playlist()->currentrevision(), el );
}

void
XspfUpdater::saveToSettings( const QString& group ) const
{
    TomahawkSettings::instance()->setValue( QString( "%1/xspfurl" ).arg( group ), m_url );
}

void
XspfUpdater::loadFromSettings( const QString& group )
{
    m_url = TomahawkSettings::instance()->value( QString( "%1/xspfurl" ).arg( group ) ).toString();
}

void
XspfUpdater::removeFromSettings( const QString& group ) const
{
    TomahawkSettings::instance()->remove( QString( "%1/xspfurl" ).arg( group ) );
}
