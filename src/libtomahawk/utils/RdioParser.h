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

#ifndef RDIOPARSER_H
#define RDIOPARSER_H

#include "jobview/JobStatusItem.h"
#include "Query.h"
#include "config.h"
#include "DropJob.h"
#include "Typedefs.h"
#include "Playlist.h"

#include <QtCore/QObject>
#include <QStringList>
#include <QSet>

#include <QNetworkRequest>

#ifdef QCA2_FOUND
#include <QtCrypto>
#endif

class NetworkReply;

namespace Tomahawk
{

class DropJobNotifier;

/**
 * Small class to parse spotify links into query_ptrs
 *
 * Connect to the signals to get the results
 */

class RdioParser : public QObject
{
    Q_OBJECT
public:

    explicit RdioParser( QObject* parent = 0 );
    virtual ~RdioParser();

    void parse( const QString& url );
    void parse( const QStringList& urls );

    void setCreatePlaylist( bool createPlaylist ) { m_createPlaylist = createPlaylist; }

signals:
    void track( const Tomahawk::query_ptr& track );
    void tracks( const QList< Tomahawk::query_ptr > tracks );

private slots:
    void expandedLinks( const QStringList& );
    void rdioReturned();

    void playlistCreated( Tomahawk::PlaylistRevision );
private:
    void parseTrack( const QString& url );
    void fetchObjectsFromUrl( const QString& url, DropJob::DropType type );

    QByteArray hmacSha1(QByteArray key, QByteArray baseString);
    QNetworkRequest generateRequest( const QString& method, const QString& url, const QList< QPair< QByteArray, QByteArray > >& extraParams, QByteArray* postData );
    QPixmap pixmap() const;
    void checkFinished();
    void parseUrl( const QString& url );

    bool m_multi;
    int m_count, m_total;
    QSet< NetworkReply* > m_reqQueries;
    DropJobNotifier* m_browseJob;

    QString m_title, m_creator;
    playlist_ptr m_playlist;

    static QPixmap* s_pixmap;

    bool m_createPlaylist;
    QList< query_ptr > m_tracks;

#ifdef QCA2_FOUND
    static QCA::Initializer m_qcaInit;
#endif
};

}

#endif // RDIOPARSER_H
