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

#ifndef Soundcloud_PARSER_H
#define Soundcloud_PARSER_H

#include <QObject>
#include <QtCore/QStringList>

#include "Typedefs.h"
#include "Query.h"
#include "DropJob.h"
#include "jobview/JobStatusItem.h"

#include "DllMacro.h"

/**
 * Small class to parse Soundcloud links into query_ptrs
 *
 * Connect to the signals to get the results
 */

class NetworkReply;

namespace Tomahawk
{

class DropJobNotifier;

class DLLEXPORT SoundcloudParser : public QObject
{
    Q_OBJECT
public:
    friend class SoundcloudJobNotifier;
    explicit SoundcloudParser( const QString& trackUrl, bool createNewPlaylist = false, QObject* parent = 0 );
    explicit SoundcloudParser( const QStringList& trackUrls, bool createNewPlaylist = false, QObject* parent = 0 );
    virtual ~SoundcloudParser();

    // if true, emits track(), if false, emits tracks().
    // only matters if you're using the QString constructor and explicityl dont' want
    // the single track signal
    void setSingleMode( bool single ) { m_single = single; }

signals:
    void track( const Tomahawk::query_ptr& track );
    void tracks( const QList< Tomahawk::query_ptr > tracks );
    void playlist( const Tomahawk::query_ptr& playlist );

private slots:
    void soundcloudArtistLookupFinished();
    void soundcloudLookupFinished();
    void playlistCreated();

private:
    QPixmap pixmap() const;
    void lookupUrl( const QString& url );
    void checkBrowseFinished();
    void parseTrack( const QVariantMap& res );

    bool m_single;
    bool m_trackMode;
    bool m_createNewPlaylist;

    int m_subscribers;
    QList< query_ptr > m_tracks;
    QVariantMap m_userData;
    bool m_getLikes;
    QSet< NetworkReply* > m_queries;
    Tomahawk::playlist_ptr m_playlist;
    DropJobNotifier* m_browseJob;
    DropJob::DropType m_type;
};

}

#endif
