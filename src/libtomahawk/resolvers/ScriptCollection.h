/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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


#ifndef SCRIPTCOLLECTION_H
#define SCRIPTCOLLECTION_H

#include "ExternalResolver.h"
#include "collection/Collection.h"
#include "collection/ArtistsRequest.h"
#include "collection/AlbumsRequest.h"

#include "Typedefs.h"
#include "DllMacro.h"


namespace Tomahawk
{

class DLLEXPORT ScriptCollection : public Collection
{
    Q_OBJECT

public:
    explicit ScriptCollection( const source_ptr& source,
                               ExternalResolver* resolver,
                               QObject* parent = 0 );
    virtual ~ScriptCollection();

    virtual QString prettyName() const;
    virtual QString itemName() const;
    virtual BackendType backendType() const { return ScriptCollectionType; }
    virtual QIcon icon() const;
    virtual QPixmap bigIcon() const;

    virtual ExternalResolver* resolver() { return m_resolver; }

    virtual Tomahawk::ArtistsRequest* requestArtists();
    virtual Tomahawk::AlbumsRequest*  requestAlbums( const Tomahawk::artist_ptr& artist );
    virtual Tomahawk::TracksRequest*  requestTracks( const Tomahawk::album_ptr& album );

private:
    ExternalResolver* m_resolver;
};

} //ns

#endif // SCRIPTCOLLECTION_H
