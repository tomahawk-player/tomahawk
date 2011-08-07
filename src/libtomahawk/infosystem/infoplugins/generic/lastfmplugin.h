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

#include "infosystem/infosystem.h"
#include "infosystem/infosystemworker.h"
#include "result.h"

#include <lastfm/Track>
#include <lastfm/Audioscrobbler>
#include <lastfm/ScrobblePoint>

#include <QObject>

class QNetworkReply;

namespace Tomahawk
{

namespace InfoSystem
{

class LastFmPlugin : public InfoPlugin
{
    Q_OBJECT

public:
    LastFmPlugin();
    virtual ~LastFmPlugin();

public slots:
    void settingsChanged();

    void onAuthenticated();
    void coverArtReturned();
    void artistImagesReturned();
    void similarArtistsReturned();
    void topTracksReturned();

    void namChangedSlot( QNetworkAccessManager *nam );

protected slots:
    virtual void getInfo( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData );
    virtual void notInCacheSlot( uint requestId, Tomahawk::InfoSystem::InfoCriteriaHash criteria, Tomahawk::InfoSystem::InfoRequestData requestData );

    virtual void pushInfo( QString caller, Tomahawk::InfoSystem::InfoType type, QVariant data );

private:
    void fetchCoverArt( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData );
    void fetchArtistImages( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData );
    void fetchSimilarArtists( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData );
    void fetchTopTracks( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData );

    void createScrobbler();
    void nowPlaying( const QVariant &input );
    void scrobble();
    void sendLoveSong( const InfoType type, QVariant input );

    void dataError( uint requestId, Tomahawk::InfoSystem::InfoRequestData requestData );

    lastfm::MutableTrack m_track;
    lastfm::Audioscrobbler* m_scrobbler;
    QString m_pw;

    QList< QUrl > m_badUrls;
};

}

}

#endif // LASTFMPLUGIN_H
