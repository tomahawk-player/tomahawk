/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
 *   Copyright 2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef M3U_LOADER_H
#define M3U_LOADER_H


#include <QtCore/QFileInfo>
#include "DllMacro.h"
#include "Typedefs.h"
#include "Query.h"
#include "Playlist.h"
#include <QObject>
#include <QSet>
#include <QFile>
#include <QtCore/QStringList>

class TrackModel;

namespace Tomahawk
{

class DLLEXPORT M3uLoader : public QObject
{
    Q_OBJECT

public:
    explicit M3uLoader( const QString& url, bool createNewPlaylist = false, QObject* parent = 0 );
    explicit M3uLoader( const QStringList& urls, bool createNewPlaylist = false, QObject* parent = 0 );
    virtual ~M3uLoader();

public slots:
    void parse();
private slots:
    void playlistCreated();

signals:
    void track( const Tomahawk::query_ptr& track );
    void tracks( const QList< Tomahawk::query_ptr > tracks );

private:
    void parseM3u( const QString& track );
    void getTags( const QFileInfo& info );
    void parseLine(const QString& line , const QFile &file);
    QList< query_ptr > m_tracks;
    QString m_title, m_info, m_creator;
    bool m_single;
    bool m_trackMode;
    bool m_createNewPlaylist;
    playlist_ptr m_playlist;
    QStringList m_urls;
};

}

#endif
