/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "ScannerStatusItem.h"

#include "utils/TomahawkUtilsGui.h"

#include "JobStatusModel.h"
#include "JobStatusView.h"
#include "filemetadata/ScanManager.h"
#include "Source.h"
#include "Track.h"
#include "utils/Logger.h"


ScannerStatusItem::ScannerStatusItem()
    : JobStatusItem()
    , m_scannedFiles( 0 )
{
    connect( ScanManager::instance(), SIGNAL( progress( unsigned int ) ), SLOT( onProgress( unsigned int ) ) );
    connect( ScanManager::instance(), SIGNAL( finished() ), SIGNAL( finished() ) );
}


ScannerStatusItem::~ScannerStatusItem()
{
}


QString
ScannerStatusItem::rightColumnText() const
{
    return QString( "%1" ).arg( m_scannedFiles );
}


QString
ScannerStatusItem::mainText() const
{
    return tr( "Scanning Collection" );
}


QPixmap
ScannerStatusItem::icon() const
{
    return TomahawkUtils::defaultPixmap( TomahawkUtils::Search );
}


void
ScannerStatusItem::onProgress( unsigned int files )
{
    m_scannedFiles = files;
    emit statusChanged();
}


ScannerStatusManager::ScannerStatusManager( QObject* parent )
    : QObject( parent )
{
    connect( ScanManager::instance(), SIGNAL( progress( unsigned int ) ), SLOT( onProgress( unsigned int ) ) );
}


void
ScannerStatusManager::onProgress( unsigned int files )
{
    Q_UNUSED( files );

    if ( !m_curItem )
    {
        // No current query item and we're resolving something, so show it
        m_curItem = QPointer< ScannerStatusItem >( new ScannerStatusItem() );
        JobStatusView::instance()->model()->addJob( m_curItem.data() );
    }
}
