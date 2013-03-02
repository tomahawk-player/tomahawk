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

#include "ScriptCollectionItem.h"

#include "audio/AudioEngine.h"
#include "ViewManager.h"

ScriptCollectionItem::ScriptCollectionItem( SourcesModel* model,
                                            SourceTreeItem* parent,
                                            const Tomahawk::collection_ptr& collection )
    : SourceTreeItem( model, parent, SourcesModel::ScriptCollection )
    , m_collection( collection )
    , m_page( 0 )
{}


ScriptCollectionItem::~ScriptCollectionItem()
{
    model()->removeSourceItemLink( this );
    ViewManager::instance()->destroyPage( m_page );
    dynamic_cast< QObject* >( m_page )->deleteLater();
}


void
ScriptCollectionItem::activate()
{
    m_page = ViewManager::instance()->show( m_collection );
    model()->linkSourceItemToPage( this, m_page );
}


QString
ScriptCollectionItem::text() const
{
    return m_collection->prettyName();
}


QString
ScriptCollectionItem::tooltip() const
{
    return m_collection->prettyName();
}


QIcon
ScriptCollectionItem::icon() const
{
    return m_collection->icon();
}


bool
ScriptCollectionItem::isBeingPlayed() const
{
    if ( m_page )
    {
        if ( m_page->isBeingPlayed() )
            return true;

        if ( !m_page->playlistInterface().isNull() &&
             m_page->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
            return true;

        if ( !m_page->playlistInterface().isNull() &&
             m_page->playlistInterface()->hasChildInterface( AudioEngine::instance()->currentTrackPlaylist() ) )
            return true;
    }
    return false;
}


Tomahawk::collection_ptr
ScriptCollectionItem::collection() const
{
    return m_collection;
}

