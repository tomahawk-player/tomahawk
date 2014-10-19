/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014,      Uwe L. Korn <uwelk@xhochy.com>
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


#ifndef TOMAHAWK_EXTERNALRESOLVERPLAYLISTUPDATERFACTORY_H
#define TOMAHAWK_EXTERNALRESOLVERPLAYLISTUPDATERFACTORY_H

#include "playlist/PlaylistUpdaterInterface.h"

#include "DllMacro.h"

namespace Tomahawk {

class DLLEXPORT ExternalResolverPlaylistUpdaterFactory : public PlaylistUpdaterFactory
{
    Q_OBJECT
public:
    ExternalResolverPlaylistUpdaterFactory();
    virtual ~ExternalResolverPlaylistUpdaterFactory();

    QString type() const override;
    PlaylistUpdaterInterface* create( const playlist_ptr&, const QVariantHash& settings ) override;

private slots:
    void resolverAdded( Tomahawk::Resolver* resolver );
};

} // namespace Tomahawk

#endif // TOMAHAWK_EXTERNALRESOLVERPLAYLISTUPDATERFACTORY_H
