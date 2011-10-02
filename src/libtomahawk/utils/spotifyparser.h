/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
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

#ifndef SPOTIFY_PARSER_H
#define SPOTIFY_PARSER_H

#include "dllmacro.h"
#include "typedefs.h"
#include "query.h"
#include "jobview/JobStatusItem.h"

#include <QObject>
#include <QSet>
#include <QtCore/QStringList>

#define SPOTIFY_PLAYLIST_API_URL "http://spotikea.tomahawk-player.org:10380"

/**
 * Small class to parse spotify links into query_ptrs
 *
 * Connect to the signals to get the results
 */

class QNetworkReply;
namespace Tomahawk
{

class DropJobNotifier;

class DLLEXPORT SpotifyParser : public QObject
{
    Q_OBJECT
public:
    friend class SpotifyJobNotifier;
    explicit SpotifyParser( const QString& trackUrl, bool createNewPlaylist = false, QObject* parent = 0 );
    explicit SpotifyParser( const QStringList& trackUrls, bool createNewPlaylist = false, QObject* parent = 0 );
    virtual ~SpotifyParser();


signals:
    void track( const Tomahawk::query_ptr& track );
    void tracks( const QList< Tomahawk::query_ptr > tracks );
    void playlist( const Tomahawk::query_ptr& playlist );

private slots:
    void spotifyTrackLookupFinished();
    void spotifyBrowseFinished();

private:
    QPixmap pixmap() const;

    void lookupUrl( const QString& url );
    void lookupTrack( const QString& track );
    void lookupSpotifyBrowse( const QString& playlist );
    void checkTrackFinished();
    void checkBrowseFinished();

    bool m_single;
    bool m_trackMode;
    bool m_createNewPlaylist;
    QList< query_ptr > m_tracks;
    QSet< QNetworkReply* > m_queries;
    QString m_title, m_info, m_creator;
    Tomahawk::playlist_ptr m_playlist;
    DropJobNotifier* m_browseJob;

    static QPixmap* s_pixmap;
};

}

#endif
