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
    explicit SpotifyParser( const QString& trackUrl, QObject* parent = 0 );
    explicit SpotifyParser( const QStringList& trackUrls, QObject* parent = 0 );
    virtual ~SpotifyParser();

signals:
    void track( const Tomahawk::query_ptr& track );
    void tracks( const QList< Tomahawk::query_ptr > tracks );

private slots:
    void spotifyTrackLookupFinished();

private:
    void lookupTrack( const QString& track );
    void checkFinished();

    bool m_single;
    QList< query_ptr > m_tracks;
    QSet< QNetworkReply* > m_queries;
};

}

#endif
