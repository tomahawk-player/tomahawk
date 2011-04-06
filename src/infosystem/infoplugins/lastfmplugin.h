/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef LASTFMPLUGIN_H
#define LASTFMPLUGIN_H
#include "tomahawk/infosystem.h"
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
    LastFmPlugin( QObject *parent );
    virtual ~LastFmPlugin();
    
    void getInfo( const QString &caller, const InfoType type, const QVariant &data, InfoCustomData customData );
    
public slots:
    void settingsChanged();
    void onAuthenticated();
    void coverArtReturned();
    virtual void notInCacheSlot( Tomahawk::InfoSystem::InfoCacheCriteria criteria, QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, Tomahawk::InfoSystem::InfoCustomData customData );
    
private:
    void fetchCoverArt( const QString &caller, const InfoType type, const QVariant& data, Tomahawk::InfoSystem::InfoCustomData &customData );
    void scrobble( const QString &caller, const InfoType type, const QVariant& data, InfoCustomData &customData );
    void createScrobbler();
    void nowPlaying( const QString &caller, const InfoType type, const QVariant& data, InfoCustomData &customData );
    void dataError( const QString &caller, const InfoType type, const QVariant& data, InfoCustomData &customData );
    
    lastfm::MutableTrack m_track;
    lastfm::Audioscrobbler* m_scrobbler;
    QString m_pw;
    
    QNetworkReply* m_authJob;
};

}

}

#endif // LASTFMPLUGIN_H
