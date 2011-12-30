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

#ifndef TOMAHAWKALBUM_H
#define TOMAHAWKALBUM_H

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

#include "typedefs.h"
#include "playlistinterface.h"
#include "dllmacro.h"

namespace Tomahawk
{

class DLLEXPORT Album : public QObject
{
Q_OBJECT

public:
    static album_ptr get( const Tomahawk::artist_ptr& artist, const QString& name, bool autoCreate = false );
    static album_ptr get( unsigned int id, const QString& name, const Tomahawk::artist_ptr& artist );

    Album( unsigned int id, const QString& name, const Tomahawk::artist_ptr& artist );
    virtual ~Album();

    unsigned int id() const { return m_id; }
    QString name() const { return m_name; }
    artist_ptr artist() const;

    Tomahawk::playlistinterface_ptr getPlaylistInterface();

signals:
    void tracksAdded( const QList<Tomahawk::query_ptr>& tracks );

private slots:
    void onTracksAdded( const QList<Tomahawk::query_ptr>& tracks );

private:
    Q_DISABLE_COPY( Album )
    Album();

    unsigned int m_id;
    QString m_name;

    artist_ptr m_artist;

    Tomahawk::playlistinterface_ptr m_playlistInterface;
};

} // ns

#endif
