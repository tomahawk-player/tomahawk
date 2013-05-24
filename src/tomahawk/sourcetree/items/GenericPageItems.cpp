/*
 *    Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "GenericPageItems.h"

#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "ViewManager.h"
#include "audio/AudioEngine.h"

using namespace Tomahawk;


/// Generic page item
GenericPageItem::GenericPageItem( SourcesModel* model, SourceTreeItem* parent, const QString& text, const QIcon& icon, boost::function< ViewPage* () > show, boost::function< ViewPage* () > get )
    : SourceTreeItem( model, parent, SourcesModel::GenericPage )
    , m_icon( icon )
    , m_text( text )
    , m_sortValue( 0 )
    , m_show( show )
    , m_get( get )
{
    if( ViewPage* p = m_get() )
        model->linkSourceItemToPage( this, p );
}


GenericPageItem::~GenericPageItem()
{
}


void
GenericPageItem::activate()
{
    ViewPage* p = m_show();
    model()->linkSourceItemToPage( this, p );
}


QString
GenericPageItem::text() const
{
    return m_text;
}


QIcon
GenericPageItem::icon() const
{
    return m_icon;
}


bool
GenericPageItem::willAcceptDrag(const QMimeData* data) const
{
    Q_UNUSED( data );
    return false;
}


void
GenericPageItem::setText( const QString &text )
{
    m_text = text;
    emit updated();
}

bool
GenericPageItem::isBeingPlayed() const
{
    if ( m_get() )
    {
        if ( m_get()->isBeingPlayed() )
            return true;

        if ( !m_get()->playlistInterface().isNull() && m_get()->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
            return true;

        if ( !m_get()->playlistInterface().isNull() && m_get()->playlistInterface()->hasChildInterface( AudioEngine::instance()->currentTrackPlaylist() ) )
            return true;
    }

    return false;
}


int
GenericPageItem::peerSortValue() const
{
    return m_sortValue;
}


void
GenericPageItem::setSortValue(int value)
{
    m_sortValue = value;
}
