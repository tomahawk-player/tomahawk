/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#ifndef DATABASECOMMAND_ALLALBUMS_H
#define DATABASECOMMAND_ALLALBUMS_H

#include <QObject>
#include <QVariantMap>

#include "Album.h"
#include "Artist.h"
#include "collection/Collection.h"
#include "collection/AlbumsRequest.h"
#include "Typedefs.h"
#include "DatabaseCommand.h"
#include "Database.h"

#include "DllMacro.h"

class DLLEXPORT DatabaseCommand_AllAlbums : public DatabaseCommand, public Tomahawk::AlbumsRequest
{
Q_OBJECT
public:
    enum SortOrder {
        None = 0,
        ModificationTime = 1
    };

    explicit DatabaseCommand_AllAlbums( const Tomahawk::collection_ptr& collection = Tomahawk::collection_ptr(),
                                        const Tomahawk::artist_ptr& artist = Tomahawk::artist_ptr(),
                                        QObject* parent = 0 );
    virtual ~DatabaseCommand_AllAlbums();

    virtual void exec( DatabaseImpl* );

    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "allalbums"; }

    virtual void enqueue() { Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( this ) ); }

    Tomahawk::collection_ptr collection() const { return m_collection; }

    void execForCollection( DatabaseImpl* );
    void execForArtist( DatabaseImpl* );

    void setArtist( const Tomahawk::artist_ptr& artist );
    void setLimit( unsigned int amount ) { m_amount = amount; }
    void setSortOrder( DatabaseCommand_AllAlbums::SortOrder order ) { m_sortOrder = order; }
    void setSortDescending( bool descending ) { m_sortDescending = descending; }
    void setFilter( const QString& filter ) { m_filter = filter; }

signals:
    void albums( const QList<Tomahawk::album_ptr>&, const QVariant& data );
    void albums( const QList<Tomahawk::album_ptr>& );
    void done();

private:
    Tomahawk::collection_ptr m_collection;
    Tomahawk::artist_ptr m_artist;

    unsigned int m_amount;
    DatabaseCommand_AllAlbums::SortOrder m_sortOrder;
    bool m_sortDescending;
    QString m_filter;
};

#endif // DATABASECOMMAND_ALLALBUMS_H
