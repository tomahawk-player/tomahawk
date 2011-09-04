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
                All =       0x10
            };

        Q_DECLARE_FLAGS(DropTypes, DropType)

        enum DropAction {

                  Append =     0x0,
                  Create =     0x1

              };
        Q_DECLARE_FLAGS(DropActions, DropAction)


    static bool acceptsMimeData( const QMimeData* data, DropJob::DropTypes type = All, DropJob::DropActions action = Append );
    static QStringList mimeTypes();

    virtual void setDropTypes( DropTypes types ) { m_dropTypes = types; }
    virtual void setDropAction( DropAction action ) { m_dropAction = action; }
    virtual DropTypes dropTypes() const { return m_dropTypes; }
    virtual DropAction dropAction() const { return m_dropAction; }
    void parseMimeData( const QMimeData* data );
    void setGetWholeArtists( bool getWholeArtists );
    void setGetWholeAlbums( bool getWholeAlbums );
    void tracksFromMimeData( const QMimeData* data, bool allowDuplicates = false, bool onlyLocal = false, bool top10 = false );
    void handleXspf( const QString& file, bool createNewPlaylist = false );
    void handleSpPlaylist( const QString& url, bool createNewPlaylist = false );

signals:
    /// QMimeData parsing results
    void tracks( const QList< Tomahawk::query_ptr >& tracks );

private slots:
    void expandedUrls( QStringList );

    void onTracksAdded( const QList<Tomahawk::query_ptr>& );

    void infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );

private:
    /// handle parsing mime data


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
    bool m_allowDuplicates;
    bool m_onlyLocal;
    bool m_getWholeArtists;
    bool m_getWholeAlbums;
    bool m_top10;
    DropTypes m_dropTypes;
    DropAction m_dropAction;
    bool DropAction();

    QList< Tomahawk::query_ptr > m_resultList;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(DropJob::DropTypes)
#endif // DROPJOB_H
