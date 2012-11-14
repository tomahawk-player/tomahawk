/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
 *   Copyright 2011, Stefan Derkits <stefan@derkits.at>
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

#ifndef GROOVESHARKPARSER_H
#define GROOVESHARKPARSER_H

#include "DllMacro.h"
#include "Typedefs.h"
#include "Query.h"
#include "jobview/JobStatusItem.h"

#include <qca_basic.h>

#include <QObject>
#include <QSet>
#include <QtCore/QStringList>

/**
 * Small class to parse grooveshark links into query_ptrs
 *
 * Connect to the signals to get the results
 */

class NetworkReply;

namespace Tomahawk
{

class DropJobNotifier;

class DLLEXPORT GroovesharkParser : public QObject
{
    Q_OBJECT
public:
    explicit GroovesharkParser( const QStringList& trackUrls, bool createNewPlaylist = false, QObject* parent = 0 );
    virtual ~GroovesharkParser();
signals:
    void track( const Tomahawk::query_ptr& track );
    void tracks( const QList< Tomahawk::query_ptr > tracks );
    void playlist( const Tomahawk::query_ptr& playlist );

private slots:
    void groovesharkLookupFinished();
    void trackPageFetchFinished();

    void playlistCreated();
private:
    QPixmap pixmap() const;

    void lookupUrl( const QString& url );
    void lookupGroovesharkPlaylist( const QString& playlist );
    void lookupGroovesharkTrack( const QString& track );

    void checkTrackFinished();
    void checkPlaylistFinished();
    int  m_limit;
    bool m_trackMode;
    bool m_createNewPlaylist;
    QList< query_ptr > m_tracks;
    QSet< NetworkReply* > m_queries;
    QString m_title, m_info, m_creator;
    Tomahawk::playlist_ptr m_playlist;
    DropJobNotifier* m_browseJob;

    QCA::SymmetricKey m_apiKey;

    static QPixmap* s_pixmap;
};

}

#endif // GROOVESHARKPARSER_H
