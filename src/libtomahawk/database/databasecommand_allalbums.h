/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "typedefs.h"
#include "databasecommand.h"
#include "album.h"

#include "dllmacro.h"

class DLLEXPORT DatabaseCommand_AllAlbums : public DatabaseCommand
{
Q_OBJECT
public:
    enum SortOrder {
        None = 0,
        ModificationTime = 1
    };

    explicit DatabaseCommand_AllAlbums( const Tomahawk::collection_ptr& collection = Tomahawk::collection_ptr(), const Tomahawk::artist_ptr& artist = Tomahawk::artist_ptr(), QObject* parent = 0 )
        : DatabaseCommand( parent )
        , m_collection( collection )
        , m_artist( artist )
        , m_amount( 0 )
        , m_sortOrder( DatabaseCommand_AllAlbums::None )
        , m_sortDescending( false )
    {}

    virtual void exec( DatabaseImpl* );

    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "allalbums"; }

    void execForCollection( DatabaseImpl* );
    void execForArtist( DatabaseImpl* );

    void setArtist( const Tomahawk::artist_ptr& artist ) { m_artist = artist; }
    void setLimit( unsigned int amount ) { m_amount = amount; }
    void setSortOrder( DatabaseCommand_AllAlbums::SortOrder order ) { m_sortOrder = order; }
    void setSortDescending( bool descending ) { m_sortDescending = descending; }
    void setFilter( const QString& filter ) { m_filter = filter; }

signals:
    void albums( const QList<Tomahawk::album_ptr>&, const QVariant& data );
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
