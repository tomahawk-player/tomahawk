/*
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

#include "genericpageitems.h"

#include "utils/tomahawkutils.h"
#include "viewmanager.h"

using namespace Tomahawk;

/// Generic page item
GenericPageItem::GenericPageItem( SourcesModel* model, SourceTreeItem* parent, const QString& text, const QIcon& icon, boost::function< ViewPage* () > show, boost::function< ViewPage* () > get )
    : SourceTreeItem( model, parent, SourcesModel::GenericPage )
    , m_icon( icon )
    , m_text( text )
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
    return false;
}

void
GenericPageItem::setText( const QString &text )
{
    m_text = text;
    emit updated();
}
