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
#ifndef PLAYLISTTEMPLATE_H
#define PLAYLISTTEMPLATE_H

#include "DllMacro.h"
#include "Typedefs.h"

namespace Tomahawk
{

class PlaylistTemplatePrivate;

class DLLEXPORT PlaylistTemplate : public QObject
{
    Q_OBJECT
public:
    explicit PlaylistTemplate( const source_ptr& author,
                               const QString& guid,
                               const QString& title,
                               const QString& info,
                               const QString& creator,
                               bool shared,
                               const QList<Tomahawk::query_ptr>& queries = QList<Tomahawk::query_ptr>());
    virtual ~PlaylistTemplate();

    /**
     * Create or get the playlist for this template.
     */
    virtual Tomahawk::playlist_ptr get();

    virtual QList<Tomahawk::query_ptr> tracks() const;

protected:
    PlaylistTemplate( PlaylistTemplatePrivate* d );
    PlaylistTemplatePrivate* d_ptr;

private:
    Q_DECLARE_PRIVATE( PlaylistTemplate )
};

}

Q_DECLARE_METATYPE( QSharedPointer< Tomahawk::PlaylistTemplate > )

#endif // PLAYLISTTEMPLATE_H
