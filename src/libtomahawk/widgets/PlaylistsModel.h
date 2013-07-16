/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#pragma once
#ifndef TOMAHAWK_PLAYLISTSMODEL_H
#define TOMAHAWK_PLAYLISTSMODEL_H

#include "DllMacro.h"
#include "Typedefs.h"

#include <QAbstractListModel>

namespace Tomahawk {

class PlaylistsModelPrivate;

class DLLEXPORT PlaylistsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit PlaylistsModel( const QList<playlist_ptr>& playlists, QObject* parent = 0 );
    virtual ~PlaylistsModel();
    
    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;

protected:
    QScopedPointer<PlaylistsModelPrivate> d_ptr;

    void updateArtists();

private:
    Q_DECLARE_PRIVATE( PlaylistsModel )

};

} // namespace Tomahawk

#endif // TOMAHAWK_PLAYLISTSMODEL_H
