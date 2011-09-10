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

#include "RadioTuner.h"
#include "../core/XmlQuery.h"
#include "../types/Xspf.h"
#include "../ws/ws.h"

using namespace lastfm;

//TODO skips left
//TODO multiple locations for the same track
//TODO set rtp flag in getPlaylist (whether user is scrobbling this radio session or not)

// limit the number of retries following empty playlists:
#define MAX_TUNING_ATTEMPTS 3


RadioTuner::RadioTuner( const RadioStation& station )
     : m_retry_counter( 0 ), m_fetchingPlaylist( false ), m_requestedPlaylist(false)
{
    m_twoSecondTimer = new QTimer( this );
    m_twoSecondTimer->setSingleShot( true );
    connect( m_twoSecondTimer, SIGNAL(timeout()), SLOT(onTwoSecondTimeout()));

    qDebug() << station.url();

    //Empty RadioStation implies that the radio
    //should tune to the previous station.
    if( station.url().isEmpty() )
    {
        fetchFiveMoreTracks();
    }
    else
    {
        QMap<QString, QString> map;
        map["method"] = "radio.tune";
        map["station"] = station.url();
        map["additional_info"] = "1";
        connect( ws::post(map), SIGNAL(finished()), SLOT(onTuneReturn()) );
    }
}

void
RadioTuner::retune( const RadioStation& station )
{
    m_playlistQueue.clear();
    m_retuneStation = station;

    qDebug() << station.url();
}


void
RadioTuner::onTuneReturn()
{
    try {
        XmlQuery lfm = qobject_cast<QNetworkReply*>(sender())->readAll();
        emit title( lfm["station"]["name"].text() );

        qDebug() << lfm;

        emit supportsDisco( lfm["station"]["supportsdiscovery"].text() == "1" );
        fetchFiveMoreTracks();
    }
    catch (ws::ParseError& e)
    {
        emit error( e.enumValue(), e.message() );
    }
}


void
RadioTuner::fetchFiveMoreTracks()
{
    if ( !m_retuneStation.url().isEmpty() )
    {
        // We have been asked to retune so do it now
        QMap<QString, QString> map;
        map["method"] = "radio.tune";
        map["station"] = m_retuneStation.url();
        map["additional_info"] = "1";

        QNetworkReply* reply = ws::post(map);
        connect( reply, SIGNAL(finished()), SLOT(onTuneReturn()) );

        m_retuneStation = RadioStation();
        m_twoSecondTimer->stop();
    }
    else
    {
        if ( !m_twoSecondTimer->isActive() )
        {
            //TODO check documentation, I figure this needs a session key
            QMap<QString, QString> map;
            map["method"] = "radio.getPlaylist";
            map["additional_info"] = "1";
            map["rtp"] = "1"; // see above
            connect( ws::post( map ), SIGNAL(finished()), SLOT(onGetPlaylistReturn()) );
            m_fetchingPlaylist = true;
        }
        else
            m_requestedPlaylist = true;
    }
}


bool
RadioTuner::tryAgain()
{
    qDebug() << "Bad response count" << m_retry_counter;
    
    if (++m_retry_counter > MAX_TUNING_ATTEMPTS)
        return false;
    fetchFiveMoreTracks();
    return true;
}


void
RadioTuner::onGetPlaylistReturn()
{   
    // We shouldn't request another playlist for 2 seconds because we'll get the same one
    // in a different order. This QTimer will block until it has finished. If one or more
    // playlists have been requested in the meantime, it will fetch one on timeout
    m_twoSecondTimer->start( 2000 );

    // This will block us fetching two playlists at once
    m_fetchingPlaylist = false;

    try {
        XmlQuery lfm = qobject_cast<QNetworkReply*>(sender())->readAll();
        emit title( lfm["playlist"]["title"].text() );

        qDebug() << lfm;

        Xspf* xspf = new Xspf( lfm["playlist"], this );
        connect( xspf, SIGNAL(expired()), SLOT(onXspfExpired()) );

        if ( xspf->isEmpty() )
        {
            // give up after too many empty playlists  :(
            if (!tryAgain())
                emit error( ws::NotEnoughContent, tr("Not enough content") );
        }
        else
        {
            m_retry_counter = 0;
            m_playlistQueue << xspf;
            emit trackAvailable();
        }
    }
    catch (ws::ParseError& e) 
    {
        qWarning() << e.what();
        emit error( e.enumValue(), e.message() );
    }
}

void
RadioTuner::onTwoSecondTimeout()
{
    if (m_requestedPlaylist)
    {
        m_requestedPlaylist = false;
        fetchFiveMoreTracks();
    }
}

void
RadioTuner::onXspfExpired()
{
    int index = m_playlistQueue.indexOf( static_cast<Xspf*>(sender()) );
    if ( index != -1 )
        m_playlistQueue.takeAt( index )->deleteLater();
}

Track
RadioTuner::takeNextTrack()
{
    if ( m_playlistQueue.isEmpty() )
    {
        // If there are no tracks here and we're not fetching tracks
        // it's probably because the playlist expired so fetch more now
        if ( !m_fetchingPlaylist )
            fetchFiveMoreTracks();

        return Track();
    }

    Track result = m_playlistQueue[0]->takeFirst();

    if ( m_playlistQueue[0]->isEmpty() )
        m_playlistQueue.removeFirst();

    if ( m_playlistQueue.isEmpty() )
        fetchFiveMoreTracks();

    return result;
}
