/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by *   the Free Software Foundation, either version 3 of the License, or
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

#ifndef LASTFMPLUGIN_H
#define LASTFMPLUGIN_H

#include "infosystem/InfoSystem.h"
#include "infosystem/InfoSystemWorker.h"

#include <lastfm/Track>
#include <lastfm/Audioscrobbler>
#include <lastfm/ScrobblePoint>

#include <QObject>

class QNetworkReply;

namespace Tomahawk
{

namespace Accounts
{
    class LastFmAccount;
}

namespace InfoSystem
{

class LastFmPlugin : public InfoPlugin
{
    Q_OBJECT

public:
    LastFmPlugin( Accounts::LastFmAccount* account );
    virtual ~LastFmPlugin();

public slots:
    void init();
    void settingsChanged();

    void onAuthenticated();
    void coverArtReturned();
    void artistImagesReturned();
    void similarArtistsReturned();
    void topTracksReturned();
    void chartReturned();

protected slots:
    virtual void getInfo( Tomahawk::InfoSystem::InfoRequestData requestData );
    virtual void notInCacheSlot( Tomahawk::InfoSystem::InfoStringHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData );

    virtual void pushInfo( Tomahawk::InfoSystem::InfoPushData pushData );

private:
    void fetchCoverArt( Tomahawk::InfoSystem::InfoRequestData requestData );
    void fetchArtistImages( Tomahawk::InfoSystem::InfoRequestData requestData );
    void fetchSimilarArtists( Tomahawk::InfoSystem::InfoRequestData requestData );
    void fetchTopTracks( Tomahawk::InfoSystem::InfoRequestData requestData );
    void fetchChart( Tomahawk::InfoSystem::InfoRequestData requestData );
    void fetchChartCapabilities( Tomahawk::InfoSystem::InfoRequestData requestData );

    void createScrobbler();
    void nowPlaying( const QVariant &input );
    void scrobble();
    void sendLoveSong( const InfoType type, QVariant input );

    void dataError( Tomahawk::InfoSystem::InfoRequestData requestData );

    QWeakPointer< Accounts::LastFmAccount > m_account;
    QList<lastfm::Track> parseTrackList( QNetworkReply * reply );

    lastfm::MutableTrack m_track;
    lastfm::Audioscrobbler* m_scrobbler;
    QString m_pw;

    QList< QUrl > m_badUrls;
};

}

}

#endif // LASTFMPLUGIN_H
