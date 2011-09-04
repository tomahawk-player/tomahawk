/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Michael Zanetti <mzanetti@kde.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef DROPJOB_H
#define DROPJOB_H

#include "query.h"

#include "infosystem/infosystem.h"

#include <QObject>
#include <QStringList>
#include <QMimeData>

/** @class DropJob
  * Allows you to process dropped mimedata in different ways:
  * Configure the DropJob using setDropFlags() or the set*() functions to do
  * what you want and then feed it with MimeMata. Connect to the tracks() signal
  * to receive the results.
  *
  * Possible configuration flags are:
  * - DropFlagTrack: Get the dropped track (only valid if the dropped item is acutally a track)
  * - DropFlagAlbum: Get this album (only valid if the dropped item is an album or a track with album information)
  * - DropFlagArtist: Get this artist
  * - DropFlagTop10: Query the Top 10 for this artist in the Network
  * - DropFlagLocal: Only get local items (Filters out all remote ones)
  * - DropFlagAllowDuplicates: Allow duplicate results, e.g. same song from different sources.
  *
  * Note: The largest possible set of the configured Flags applies. E.g. Artist is greater than Album.
  * If you set both of them only the album will be fetched. Requesting the Top 10 items always results in a
  * query for the whole artist. It is not possible to e.g. request the Top 10 tracks of a given album.
  *
  * If you configure nothing or dropping incompatible data (e.g. configured DropTrack but dropping an album),
  * the DropJob will do this default actions:
  * - Get this track for dropped tracks
  * - Get whole album for dropped albums
  * - Get whole artist for dropped artists
  */

class DLLEXPORT DropJob : public QObject
{
    Q_OBJECT
public:
    enum DropFlag
    {
        DropFlagsNone = 0x00,
        DropFlagTrack = 0x01,
        DropFlagAlbum = 0x02,
        DropFlagArtist = 0x04,
        DropFlagTop10 = 0x08,
        DropFlagLocal = 0x10,
        DropFlagAllowDuplicates = 0x20,
        DropFlagsAll = 0xff
    };
    Q_DECLARE_FLAGS( DropFlags, DropFlag )

    explicit DropJob( QObject *parent = 0 );
    ~DropJob();

    /**
     * QMimeData helpers
     *
     *  Call this to parse the tracks in a QMimeData object to query_ptrs. This will parse internal tomahawk
     *   data as well as all other formats supported (spotify, etc).
     *
     * Connect to tracks( QList< query_ptr> ); for the extracted tracks.
     */
    static bool acceptsMimeData( const QMimeData* data, bool tracksOnly = true );
    static QStringList mimeTypes();

    void setDropFlags( DropFlags flags );

    void setGetWholeArtists( bool getWholeArtists );
    void setGetWholeAlbums( bool getWholeAlbums );
    void setGetTop10( bool top10 );
    void setOnlyLocal( bool onlyLocal );
    void setAllowDuplicates( bool allowDuplicates );

    void tracksFromMimeData( const QMimeData* data );

signals:
    /// QMimeData parsing results
    void tracks( const QList< Tomahawk::query_ptr >& tracks );

private slots:
    void expandedUrls( QStringList );

    void onTracksAdded( const QList<Tomahawk::query_ptr>& );

    void infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );

private:
    /// handle parsing mime data
    void parseMimeData( const QMimeData* data );

    void handleTrackUrls( const QString& urls );
    QList< Tomahawk::query_ptr > tracksFromQueryList( const QMimeData* d );
    QList< Tomahawk::query_ptr > tracksFromResultList( const QMimeData* d );
    QList< Tomahawk::query_ptr > tracksFromArtistMetaData( const QMimeData* d );
    QList< Tomahawk::query_ptr > tracksFromAlbumMetaData( const QMimeData* d );
    QList< Tomahawk::query_ptr > tracksFromMixedData( const QMimeData* d );

    QList< Tomahawk::query_ptr > getArtist( const QString& artist );
    QList< Tomahawk::query_ptr > getAlbum( const QString& artist, const QString& album );

    void getTopTen( const QString& artist );

    void removeDuplicates();
    void removeRemoteSources();

    int m_queryCount;
    DropFlags m_dropFlags;

    QList< Tomahawk::query_ptr > m_resultList;
};
Q_DECLARE_OPERATORS_FOR_FLAGS( DropJob::DropFlags )

#endif // DROPJOB_H
