/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "IndexingJobItem.h"

#include "utils/tomahawkutils.h"

#include <QPixmap>

static QPixmap* s_indexIcon = 0;


QString
IndexingJobItem::mainText() const
{
    return tr( "Indexing database" );
}

QPixmap
IndexingJobItem::icon() const
{
    if ( s_indexIcon == 0 )
        s_indexIcon = new QPixmap( RESPATH "images/view-refresh.png" );

    return *s_indexIcon;

}


void IndexingJobItem::done()
{
    emit finished();
}

