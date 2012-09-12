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

#include "Query.h"

#include <QObject>
#include <QStringList>
#include <QMimeData>

namespace Tomahawk {
class DropJobNotifier;
}


class DLLEXPORT DropJob : public QObject
{
    Q_OBJECT
public:
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

    enum DropType {
        None =      0x00,
        Playlist =  0x01,
        Track =     0x02,
        Album =     0x04,
        Artist =    0x08,

        All =       0xFF
    };
    Q_DECLARE_FLAGS(DropTypes, DropType)

    enum DropAction {
        Default = 0,
        Append,
        Create,
        Move
    };

    /**
     * Returns if the caller should accept this mimetype.
     *
     * \param data The mimetype object to check
     * \param type The type of drop content to accept
     * \param action What action is requested from the content, if not all data types support all actions
     */
    static bool acceptsMimeData( const QMimeData* data, DropJob::DropTypes type = All, DropAction action = Append );

    /**
     * Return if the drop is primarily of the given type. Does not auto-convert (e.g. if the drop is of type playlist,
     *  even thougha playlist can be converted into tracks, this will return true only for the Playlist drop type).
     *
     * TODO Only implemented for Playlist atm. Extend when you need it.
     */
    static bool isDropType( DropJob::DropType desired, const QMimeData* data );

    static QStringList mimeTypes();

    /// Set the drop types that should be extracted from this drop
    void setDropTypes( DropTypes types ) { m_dropTypes = types; }

    /// Set the action that the drop should do. For example, if dropping a playlist, Create will create a new playlist but Append will generate the raw tracks
    void setDropAction( DropAction action ) { m_dropAction = action; }

    DropTypes dropTypes() const { return m_dropTypes; }
    DropAction dropAction() const { return m_dropAction; }

    /**
     * Begin the parsing of the mime data. The resulting tracks are exposed in the various signals
     */
    void parseMimeData( const QMimeData* data );

    void setGetWholeArtists( bool getWholeArtists );
    void setGetWholeAlbums( bool getWholeAlbums );
    void tracksFromMimeData( const QMimeData* data, bool allowDuplicates = false, bool onlyLocal = false, bool top10 = false );
    void handleXspfs( const QString& files );
    void handleM3u( const QString& urls );
    void handleSpotifyUrls( const QString& urls );
    void handleRdioUrls( const QString& urls );
    void handleExfmUrls( const QString& urls );
    void handleSoundcloudUrls( const QString& urls );
    void handleGroovesharkUrls( const QString& urls );

    static bool canParseSpotifyPlaylists() { return s_canParseSpotifyPlaylists; }
    static void setCanParseSpotifyPlaylists( bool parseable ) { s_canParseSpotifyPlaylists = parseable; }

signals:
    /// QMimeData parsing results
    void tracks( const QList< Tomahawk::query_ptr >& tracks );

private slots:
    void expandedUrls( QStringList );
    void onTracksAdded( const QList<Tomahawk::query_ptr>& );

private:
    /// handle parsing mime data
    void handleAllUrls( const QString& urls );
    void handleTrackUrls( const QString& urls );
    QList< Tomahawk::query_ptr > tracksFromQueryList( const QMimeData* d );
    QList< Tomahawk::query_ptr > tracksFromResultList( const QMimeData* d );
    QList< Tomahawk::query_ptr > tracksFromArtistMetaData( const QMimeData* d );
    QList< Tomahawk::query_ptr > tracksFromAlbumMetaData( const QMimeData* d );
    void tracksFromMixedData( const QMimeData* d );

    QList< Tomahawk::query_ptr > getArtist( const QString& artist, Tomahawk::ModelMode mode = Tomahawk::Mixed );
    QList< Tomahawk::query_ptr > getAlbum( const QString& artist, const QString& album );
    QList< Tomahawk::query_ptr > getTopTen( const QString& artist );

    void removeDuplicates();
    void removeRemoteSources();

    int m_queryCount;
    bool m_allowDuplicates;
    bool m_onlyLocal;
    bool m_getWholeArtists;
    bool m_getWholeAlbums;
    bool m_top10;
    DropTypes m_dropTypes;
    DropAction m_dropAction;

    QList<Tomahawk::DropJobNotifier*> m_dropJob;

    QList< Tomahawk::query_ptr > m_resultList;
    QSet< Tomahawk::album_ptr > m_albumsToKeep;
    QSet< Tomahawk::artist_ptr > m_artistsToKeep;

    static bool s_canParseSpotifyPlaylists;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DropJob::DropTypes)
#endif // DROPJOB_H
