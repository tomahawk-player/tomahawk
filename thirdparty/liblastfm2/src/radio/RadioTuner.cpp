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
     : m_retry_counter( 0 )
{
    //Empty RadioStation implies that the radio
    //should tune to the previous station.
    if( station.url().isEmpty() ) {
        fetchFiveMoreTracks();
        return;
    }
    
    QMap<QString, QString> map;
    map["method"] = "radio.tune";
    map["station"] = station.url();
    map["additional_info"] = "1";
    QNetworkReply* reply = ws::post(map);
    connect( reply, SIGNAL(finished()), SLOT(onTuneReturn()) );
}

void
RadioTuner::retune( const RadioStation& station)
{
    m_queue.clear();

    QMap<QString, QString> map;
    map["method"] = "radio.tune";
    map["station"] = station.url();
    map["additional_info"] = "1";
    QNetworkReply* reply = ws::post(map);
    connect( reply, SIGNAL(finished()), SLOT(onTuneReturn()) );
}


void
RadioTuner::onTuneReturn()
{
    try {
        XmlQuery lfm = ws::parse( (QNetworkReply*)sender() );
        // TODO: uncomment this is we are to get a radio station
        // name when we tune to an rql radio station
        //emit title( lfm["station"]["name"].text() );

        qDebug() << lfm;

        emit supportsDisco( lfm["station"]["supportsdiscovery"].text() == "1" );
        fetchFiveMoreTracks();
    }
    catch (ws::ParseError& e)
    {
        emit error( e.enumValue() );
    }
}


bool
RadioTuner::fetchFiveMoreTracks()
{
    //TODO check documentation, I figure this needs a session key
    QMap<QString, QString> map;
    map["method"] = "radio.getPlaylist";
    map["additional_info"] = "1";
    map["rtp"] = "1"; // see above
    QNetworkReply* reply = ws::post( map );
    connect( reply, SIGNAL(finished()), SLOT(onGetPlaylistReturn()) );
    return true;
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
    try {
        XmlQuery lfm = ws::parse( (QNetworkReply*)sender() );
        Xspf xspf( lfm["playlist"] );
        QList<Track> tracks( xspf.tracks() );
        if (tracks.isEmpty()) {
            // give up after too many empty playlists  :(
            if (!tryAgain())
                emit error( ws::NotEnoughContent );
        } else {
            m_retry_counter = 0;
            foreach (Track t, tracks)
                MutableTrack( t ).setSource( Track::LastFmRadio );
            m_queue += tracks;
            emit trackAvailable();
        }
    }
    catch (ws::ParseError& e) 
    {
        qWarning() << e.what();
        emit error( e.enumValue() );
    }
}


Track
RadioTuner::takeNextTrack()
{
    //TODO presumably, we should check if fetchMoreTracks is working?
    if (m_queue.isEmpty())
        return Track();
    
    Track result = m_queue.takeFirst();
    if (m_queue.isEmpty())
        fetchFiveMoreTracks();

    return result;
}
