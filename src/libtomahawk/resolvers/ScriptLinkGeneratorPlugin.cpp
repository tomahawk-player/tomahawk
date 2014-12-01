/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014,    Dominik Schmidt <domme@tomahawk-player.org>
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
#include "ScriptLinkGeneratorPlugin_p.h"

#include "ScriptObject.h"

#include "../Artist.h"
#include "../Album.h"

using namespace Tomahawk;

ScriptLinkGeneratorPlugin::ScriptLinkGeneratorPlugin( ScriptObject* scriptObject )
    : Utils::LinkGeneratorPlugin()
    , d_ptr( new ScriptLinkGeneratorPluginPrivate( this, scriptObject ) )
{
}


ScriptLinkGeneratorPlugin::~ScriptLinkGeneratorPlugin()
{
}


ScriptJob*
ScriptLinkGeneratorPlugin::openLink( const QString& title, const QString& artist, const QString& album ) const
{
    Q_D( const ScriptLinkGeneratorPlugin );

    QVariantMap arguments;
    arguments[ "title" ] = QVariant( title );
    arguments[ "artist" ] = QVariant( artist );
    arguments[ "album" ] = QVariant( album );

    return d->scriptObject->invoke( "generateQueryLink", arguments );
}


ScriptJob*
ScriptLinkGeneratorPlugin::openLink( const artist_ptr& artist ) const
{
    Q_D( const ScriptLinkGeneratorPlugin );

    // TODO: create proper serializer for QObjects
    QVariantMap arguments;
    arguments[ "name" ] = QVariant( artist->name() );

    return d->scriptObject->invoke( "generateArtistLink", arguments );
}


ScriptJob*
ScriptLinkGeneratorPlugin::openLink( const album_ptr& album ) const
{
    Q_D( const ScriptLinkGeneratorPlugin );

    // TODO: create proper serializer for QObjects
    QVariantMap arguments;
    arguments[ "name" ] = QVariant( album->name() );

    return d->scriptObject->invoke( "generateAlbumLink", arguments );
}


ScriptJob*
ScriptLinkGeneratorPlugin::openLink( const dynplaylist_ptr& playlist ) const
{
    return nullptr;
}
