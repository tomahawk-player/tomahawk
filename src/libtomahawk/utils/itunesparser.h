/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Written by Hugo Lindström <hugolm84@gmail.com>
 *   But based on Leo Franchi's work from spotifyParser
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

#ifndef ITUNES_PARSER_H
#define ITUNES_PARSER_H

#include "dllmacro.h"
#include "typedefs.h"
#include "query.h"

#include <QObject>
#include <QSet>
#include <QtCore/QStringList>

class QNetworkReply;
class TrackModel;

namespace Tomahawk
{

/**
 * Small class to parse itunes links into query_ptrs
 *
 * Connect to the signals to get the results
 */
class DLLEXPORT ItunesParser : public QObject
{
    Q_OBJECT
public:
    explicit ItunesParser( const QString& trackUrl, QObject* parent = 0 );
    explicit ItunesParser( const QStringList& trackUrls, QObject* parent = 0 );
    virtual ~ItunesParser();

signals:
    void track( const Tomahawk::query_ptr& track );
    void tracks( const QList< Tomahawk::query_ptr > tracks );

private slots:
    void itunesResponseLookupFinished();

private:
    void lookupItunesUri( const QString& track );
    void checkTrackFinished();

    bool m_single;
    QList< query_ptr > m_tracks;
    QSet< QNetworkReply* > m_queries;
    QString m_title, m_info, m_creator;
    Tomahawk::playlist_ptr m_playlist;
};

}

#endif
