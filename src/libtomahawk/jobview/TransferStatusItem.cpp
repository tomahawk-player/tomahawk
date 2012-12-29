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

#include "TransferStatusItem.h"

#include "JobStatusView.h"
#include "JobStatusModel.h"
#include "Result.h"
#include "Source.h"
#include "Artist.h"
#include "network/StreamConnection.h"
#include "network/Servent.h"
#include "utils/TomahawkUtilsGui.h"


TransferStatusItem::TransferStatusItem( TransferStatusManager* p, StreamConnection* sc )
    : m_parent( p )
    , m_stream( QWeakPointer< StreamConnection >( sc ) )
{
    if ( m_stream.data()->type() == StreamConnection::RECEIVING )
        m_type = "receive";
    else
        m_type = "send";

    connect( m_stream.data(), SIGNAL( updated() ), SLOT( onTransferUpdate() ) );
    connect( Servent::instance(), SIGNAL( streamFinished( StreamConnection* ) ), SLOT( streamFinished( StreamConnection* ) ) );
}

TransferStatusItem::~TransferStatusItem()
{

}

QString
TransferStatusItem::mainText() const
{
    if ( m_stream.isNull() )
        return QString();

    if ( m_stream.data()->source().isNull() && !m_stream.data()->track().isNull() )
        return QString( "%1" ).arg( QString( "%1 - %2" ).arg( m_stream.data()->track()->artist()->name() ).arg( m_stream.data()->track()->track() ) );
    else if ( !m_stream.data()->source().isNull() && !m_stream.data()->track().isNull() )
        return QString( "%1 %2 %3" ).arg( QString( "%1 - %2" ).arg( m_stream.data()->track()->artist()->name() ).arg( m_stream.data()->track()->track() ) )
                                .arg( m_stream.data()->type() == StreamConnection::RECEIVING ? tr( "from", "streaming artist - track from friend" ) : tr( "to", "streaming artist - track to friend" ) )
                                .arg( m_stream.data()->source()->friendlyName() );
    else
        return QString();
}

QString
TransferStatusItem::rightColumnText() const
{
    if ( m_stream.isNull() )
        return QString();

    return QString( "%1 kB/s" ).arg( m_stream.data()->transferRate() / 1000 );
}

void
TransferStatusItem::streamFinished( StreamConnection* sc )
{
    if ( m_stream.data() == sc )
        emit finished();
}

QPixmap
TransferStatusItem::icon() const
{
    if ( m_stream.isNull() )
        return QPixmap();

    if ( m_stream.data()->type() == StreamConnection::SENDING )
        return m_parent->txPixmap();
    else
        return m_parent->rxPixmap();
}


void
TransferStatusItem::onTransferUpdate()
{
    emit statusChanged();
}


TransferStatusManager::TransferStatusManager( QObject* parent )
    : QObject( parent )
{
    connect( Servent::instance(), SIGNAL( streamStarted( StreamConnection* ) ), SLOT( streamRegistered( StreamConnection* ) ) );
}

void
TransferStatusManager::streamRegistered( StreamConnection* sc )
{
    JobStatusView::instance()->model()->addJob( new TransferStatusItem( this, sc ) );
}


QPixmap
TransferStatusManager::rxPixmap() const
{
    return TomahawkUtils::defaultPixmap( TomahawkUtils::Downloading, TomahawkUtils::Original, QSize( 128, 128 ) );
}


QPixmap
TransferStatusManager::txPixmap() const
{
    return TomahawkUtils::defaultPixmap( TomahawkUtils::Uploading, TomahawkUtils::Original, QSize( 128, 128 ) );
}
