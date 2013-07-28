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
#ifndef XSPFPLAYLISTTEMPLATE_P_H
#define XSPFPLAYLISTTEMPLATE_P_H

#include "playlist/PlaylistTemplate_p.h"
#include "playlist/XspfPlaylistTemplate.h"
#include "utils/XspfLoader.h"

namespace Tomahawk
{

class XspfPlaylistTemplatePrivate: public PlaylistTemplatePrivate
{
public:
    XspfPlaylistTemplatePrivate( XspfPlaylistTemplate* q, const QString& _url, const source_ptr& source, const QString& guid )
        : PlaylistTemplatePrivate( q, source, guid, QString(), QString(), QString(), false, QList<query_ptr>() )
        , url( _url )
        , xspfLoader( new XSPFLoader( false, true, 0, guid ) )
    {
    }

    Q_DECLARE_PUBLIC( XspfPlaylistTemplate )

private:
    QString url;
    QSharedPointer<XSPFLoader> xspfLoader;
};

} // namespace Tomahawk

#endif // XSPFPLAYLISTTEMPLATE_P_H
