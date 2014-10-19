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

#include "JobStatusModel.h"
#include "JobStatusView.h"
#include "database/Database.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QPixmap>


QString
IndexingJobItem::mainText() const
{
    return tr( "Indexing Music Library" );
}


QPixmap
IndexingJobItem::icon() const
{
    return TomahawkUtils::defaultPixmap( TomahawkUtils::ViewRefresh, TomahawkUtils::Original, QSize( 128, 128 ) );
}


void
IndexingJobItem::done()
{
    emit finished();
}


IndexStatusManager::IndexStatusManager( QObject* parent )
: QObject( parent )
{
    connect( Tomahawk::Database::instance(), SIGNAL( indexStarted() ), SLOT( started() ) );
    connect( Tomahawk::Database::instance(), SIGNAL( indexReady() ), SLOT( finished() ) );
}


void
IndexStatusManager::started()
{
    m_curItem = new IndexingJobItem;
    JobStatusView::instance()->model()->addJob( m_curItem.data() );
}


void
IndexStatusManager::finished()
{
    if ( m_curItem )
        m_curItem->done();
}
