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

#include <QTimer>
#include <tomahawksettings.h>

using namespace Tomahawk;

XspfUpdater::XspfUpdater( const playlist_ptr& pl, const QString& xUrl )
    : PlaylistUpdaterInterface( pl )
    , m_url( xUrl )
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
    l->load( m_url );
    connect( l, SIGNAL( ok ( Tomahawk::playlist_ptr ) ), this, SLOT( playlistLoaded() ) );
}

void
XspfUpdater::playlistLoaded()
{
    XSPFLoader* loader = qobject_cast<XSPFLoader*>( sender() );
    Q_ASSERT( loader );

    QList< query_ptr > queries = loader->entries();
    QList<plentry_ptr> el = playlist()->entriesFromQueries( queries, true );
    playlist()->createNewRevision( uuid(), playlist()->currentrevision(), el );

//    // if there are any different from the current playlist, clear and use the new one, update
//    bool changed = ( queries.size() == playlist()->entries().count() );
//    if ( !changed )
//    {
//        foreach( const query_ptr& newSong, queries )
//        {
//            if ( !playlist()->entries.contains() )
//        }
//    }
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
