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

#ifndef SPOTIFY_PARSER_H
#define SPOTIFY_PARSER_H

#include "dllmacro.h"
#include "typedefs.h"
#include "query.h"

#include <QObject>
#include <QSet>
#include <QtCore/QStringList>

class QNetworkReply;
namespace Tomahawk
{

/**
 * Small class to parse spotify links into query_ptrs
 *
 * Connect to the signals to get the results
 */
class DLLEXPORT SpotifyParser : public QObject
{
    Q_OBJECT
public:
    explicit SpotifyParser( const QString& trackUrl, QObject* parent = 0, bool createNewPl = false );
    explicit SpotifyParser( const QStringList& trackUrls, QObject* parent = 0, bool createNewPl = false);
    virtual ~SpotifyParser();

signals:
    void track( const Tomahawk::query_ptr& track );
    void tracks( const QList< Tomahawk::query_ptr > tracks );
    void playlist( const Tomahawk::query_ptr& playlist );

private slots:
    void spotifyTrackLookupFinished();
    void spotifyPlaylistLookupFinished();

private:
    void lookupUrl( const QString& url );
    void lookupTrack( const QString& track );
    void lookupPlaylist( const QString& playlist );
    void checkTrackFinished();
    void checkPlaylistFinished();
    bool m_single;
    bool m_createNewPlaylist;
    QList< query_ptr > m_tracks;
    QSet< QNetworkReply* > m_queries;
    QString m_title, m_info, m_creator;
    Tomahawk::playlist_ptr m_playlist;
};

}

#endif
