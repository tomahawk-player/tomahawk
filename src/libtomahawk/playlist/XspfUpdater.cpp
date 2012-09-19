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

#include "Playlist.h"
#include "utils/XspfLoader.h"
#include "Pipeline.h"
#include "utils/TomahawkUtils.h"
#include "Source.h"

#include <QTimer>

#ifndef ENABLE_HEADLESS
#include <QCheckBox>
#endif

using namespace Tomahawk;

PlaylistUpdaterInterface*
XspfUpdaterFactory::create( const playlist_ptr &pl, const QVariantHash& settings )
{
    const bool autoUpdate = settings.value( "autoupdate" ).toBool();
    const int interval = settings.value( "interval" ).toInt();
    const QString url = settings.value( "xspfurl" ).toString();

    XspfUpdater* updater = new XspfUpdater( pl, interval, autoUpdate, url );

    return updater;
}


XspfUpdater::XspfUpdater( const playlist_ptr& pl, int interval, bool autoUpdate, const QString& xspfUrl )
    : PlaylistUpdaterInterface( pl )
    , m_timer( new QTimer( this ) )
    , m_autoUpdate( autoUpdate )
    , m_url( xspfUrl )
{
    m_timer->setInterval( interval );

    connect( m_timer, SIGNAL( timeout() ), this, SLOT( updateNow() ) );

#ifndef ENABLE_HEADLESS
    m_toggleCheckbox = new QCheckBox( );
    m_toggleCheckbox->setText( tr( "Automatically update from XSPF" ) );
    m_toggleCheckbox->setLayoutDirection( Qt::RightToLeft );
    m_toggleCheckbox->setChecked( m_autoUpdate );
    m_toggleCheckbox->hide();

    connect( m_toggleCheckbox, SIGNAL( toggled( bool ) ), this, SLOT( setAutoUpdate( bool ) ) );
#endif

    QVariantHash s = settings();
    s[ "autoupdate" ] = m_autoUpdate;
    s[ "interval" ] = interval;
    s[ "xspfurl" ] = xspfUrl;
    saveSettings( s );

    // Force start
    setAutoUpdate( m_autoUpdate );
}


XspfUpdater::~XspfUpdater()
{
}


#ifndef ENABLE_HEADLESS

QWidget*
XspfUpdater::configurationWidget() const
{
    return m_toggleCheckbox;
}

#endif


void
XspfUpdater::updateNow()
{
    if ( m_url.isEmpty() )
    {
        qWarning() << "XspfUpdater not updating because we have an empty url...";
        return;
    }

    XSPFLoader* l = new XSPFLoader( false, false );
    l->setAutoResolveTracks( false );
    l->setErrorTitle( playlist()->title() );
    l->load( m_url );
    connect( l, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( playlistLoaded( QList<Tomahawk::query_ptr> ) ) );
}


void
XspfUpdater::playlistLoaded( const QList<Tomahawk::query_ptr>& newEntries )
{
    XSPFLoader* loader = qobject_cast< XSPFLoader* >( sender() );
    if ( loader )
    {
        const QString newTitle = loader->title();
        if ( newTitle != playlist()->title() )
            playlist()->rename( newTitle );
    }

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
XspfUpdater::setAutoUpdate( bool autoUpdate )
{
    m_autoUpdate = autoUpdate;

    if ( m_autoUpdate )
        m_timer->start();
    else
        m_timer->stop();

    QVariantHash s = settings();
    s[ "autoupdate" ] = m_autoUpdate;
    saveSettings( s );

    // Update immediately as well
    if ( m_autoUpdate )
        QTimer::singleShot( 0, this, SLOT( updateNow() ) );

    emit changed();
}

void
XspfUpdater::setInterval( int intervalMsecs )
{
    QVariantHash s = settings();
    s[ "interval" ] = intervalMsecs;
    saveSettings( s );

    if ( !m_timer )
        m_timer = new QTimer( this );

    m_timer->setInterval( intervalMsecs );
}


void
XspfUpdater::setSubscribed( bool subscribed )
{
    setAutoUpdate( subscribed );
}
