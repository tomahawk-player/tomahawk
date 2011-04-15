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

#include <QPointer>

#include "Audioscrobbler.h"
#include "ScrobbleCache.h"

#include "../types/User.h"
#include "../types/Track.h"
#include "../ws/ws.h"
#include "../core/XmlQuery.h"


namespace lastfm
{
    class AudioscrobblerPrivate
    {
    public:
        AudioscrobblerPrivate(const QString& id)
                : m_id( id )
                , m_cache( ws::Username )
        {}
        
        ~AudioscrobblerPrivate()
        {
        }

        const QString m_id;
        ScrobbleCache m_cache;
        QList<Track> m_batch;
        QPointer<QNetworkReply> m_nowPlayingReply;
        QPointer<QNetworkReply> m_scrobbleReply;
        Track m_nowPlayingTrack;
    };
}


lastfm::Audioscrobbler::Audioscrobbler( const QString& id )
        : d( new AudioscrobblerPrivate(id) )
{
    submit();
}


lastfm::Audioscrobbler::~Audioscrobbler()
{
    delete d;
}


void
lastfm::Audioscrobbler::nowPlaying( const Track& track )
{
    if ( d->m_nowPlayingReply.isNull())
    {
        d->m_nowPlayingTrack = track;
        d->m_nowPlayingReply = track.updateNowPlaying();
        connect( d->m_nowPlayingReply, SIGNAL(finished()), SLOT(onNowPlayingReturn()));
    }
}


void
lastfm::Audioscrobbler::cache( const Track& track )
{
    QList<Track> tracks;
    tracks.append( track );
    cacheBatch( tracks );
}


void
lastfm::Audioscrobbler::cacheBatch( const QList<Track>& tracks )
{
    d->m_cache.add( tracks );

    foreach ( const Track& track, d->m_cache.tracks() )
        MutableTrack( track ).setScrobbleStatus( Track::Cached );

    emit scrobblesCached( tracks );
    submit();
}


void
lastfm::Audioscrobbler::submit()
{
    if (d->m_cache.tracks().isEmpty() // there are no tracks to submit
            || !d->m_scrobbleReply.isNull() ) // we are already submitting scrobbles
        return;

    // copy tracks to be submitted to a temporary list
    d->m_batch = d->m_cache.tracks().mid( 0, 50 );

    // if there is only one track use track.scrobble, otherwise use track.scrobbleBatch
    if (d->m_batch.count() == 1)
        d->m_scrobbleReply = d->m_batch[0].scrobble();
    else
        d->m_scrobbleReply = lastfm::Track::scrobble( d->m_batch );

    connect( d->m_scrobbleReply, SIGNAL(finished()), SLOT(onTrackScrobbleReturn()));
}

void
lastfm::Audioscrobbler::parseTrack( const XmlQuery& trackXml, const Track& track )
{
    MutableTrack mTrack = MutableTrack( track );
    bool isScrobble = QDomElement(trackXml).tagName() == "scrobble";

    if ( trackXml["ignoredMessage"].attribute("code") == "0" )
    {
        if ( isScrobble ) mTrack.setScrobbleStatus( Track::Submitted );

        // corrections!
        if ( trackXml["track"].attribute("corrected") == "1"
             || trackXml["artist"].attribute("corrected") == "1"
             || trackXml["album"].attribute("corrected") == "1"
             || trackXml["albumArtist"].attribute("corrected") == "1")
        {
            mTrack.setCorrections(trackXml["track"].text(),
                                           trackXml["album"].text(),
                                           trackXml["artist"].text(),
                                           trackXml["albumArtist"].text());
        }
    }
    else if ( isScrobble )
    {
        mTrack.setScrobbleError( static_cast<Track::ScrobbleError>(trackXml["ignoredMessage"].attribute("code").toInt()) );
        mTrack.setScrobbleStatus( Track::Error );
    }
}

void
lastfm::Audioscrobbler::onNowPlayingReturn()
{
    lastfm::XmlQuery lfm = static_cast<QNetworkReply*>(sender())->readAll();
    qDebug() << lfm;

    if ( lfm.attribute("status") == "ok" )
        parseTrack( lfm["nowplaying"], d->m_nowPlayingTrack );
    else
        emit nowPlayingError( lfm["error"].attribute("code").toInt(), lfm["error"].text() );

    d->m_nowPlayingTrack = Track();
    d->m_nowPlayingReply = 0;
}


void
lastfm::Audioscrobbler::onTrackScrobbleReturn()
{
    lastfm::XmlQuery lfm = d->m_scrobbleReply->readAll();
    qDebug() << lfm;

    if (lfm.attribute("status") == "ok")
    {
        int index = 0;

        foreach ( const XmlQuery& scrobble, lfm["scrobbles"].children("scrobble") )
            parseTrack( scrobble, d->m_batch.at( index++ ) );

        d->m_cache.remove( d->m_batch );
        d->m_batch.clear();
    }
    else
    {
        // The scrobble submission failed

        if ( !(lfm["error"].attribute("code") == "9" // Bad session
            || lfm["error"].attribute("code") == "11" // Service offline
            || lfm["error"].attribute("code") == "16") ) // Service temporarily unavailable
        {
            // clear the cache if it was not one of these error codes
            d->m_cache.remove( d->m_batch );
            d->m_batch.clear();
        }
        else
        {
            qWarning() << "Got error in scrobble submission:" << lfm[ "error" ] << "and silently ignoring. Submission is cached.";
            //Q_ASSERT(false);
        }
    }

    d->m_scrobbleReply = 0;
}
