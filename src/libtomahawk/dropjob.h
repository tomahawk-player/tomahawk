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
    static bool acceptsMimeData( const QMimeData* data, bool tracksOnly = true );
    static QStringList mimeTypes();
    void tracksFromMimeData( const QMimeData* data );

signals:
    /// QMimeData parsing results
    void tracks( const QList< Tomahawk::query_ptr >& tracks );

private slots:
    void expandedUrls( QStringList );

    void onTracksAdded( const QList<Tomahawk::query_ptr>& );

private:
    /// handle parsing mime data
    void handleTrackUrls( const QString& urls );
    QList< Tomahawk::query_ptr > tracksFromQueryList( const QMimeData* d );
    QList< Tomahawk::query_ptr > tracksFromResultList( const QMimeData* d );
    QList< Tomahawk::query_ptr > tracksFromArtistMetaData( const QMimeData* d );
    QList< Tomahawk::query_ptr > tracksFromAlbumMetaData( const QMimeData* d );
    QList< Tomahawk::query_ptr > tracksFromMixedData( const QMimeData* d );

    int m_queryCount;
};

#endif // DROPJOB_H
