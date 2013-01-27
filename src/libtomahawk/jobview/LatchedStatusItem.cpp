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

#include "LatchedStatusItem.h"

#include "Source.h"
#include "SourceList.h"
#include "JobStatusView.h"
#include "JobStatusModel.h"
#include "utils/TomahawkUtilsGui.h"

#ifndef ENABLE_HEADLESS
#include "JobStatusModel.h"
#include "JobStatusView.h"
#endif

LatchedStatusItem::LatchedStatusItem( const Tomahawk::source_ptr& from, const Tomahawk::source_ptr& to, LatchedStatusManager* parent )
    : JobStatusItem()
    , m_from( from )
    , m_to( to )
    , m_parent( parent )
{
    m_text = tr( "%1 is listening along with you!" ).arg( from->friendlyName() );
}

LatchedStatusItem::~LatchedStatusItem()
{
}

QPixmap
LatchedStatusItem::icon() const
{
    return m_parent->pixmap();
}

QString
LatchedStatusItem::mainText() const
{
    return m_text;
}

QString
LatchedStatusItem::type() const
{
    return "latched";
}

void LatchedStatusItem::stop()
{
    emit finished();
}

LatchedStatusManager::LatchedStatusManager( QObject* parent )
    : QObject( parent )
{
    connect( SourceList::instance(), SIGNAL( sourceLatchedOn( Tomahawk::source_ptr, Tomahawk::source_ptr ) ), this, SLOT( latchedOn( Tomahawk::source_ptr, Tomahawk::source_ptr ) ) );
    connect( SourceList::instance(), SIGNAL( sourceLatchedOff( Tomahawk::source_ptr, Tomahawk::source_ptr ) ), this, SLOT( latchedOff( Tomahawk::source_ptr, Tomahawk::source_ptr ) ) );
}

void
LatchedStatusManager::latchedOn( const Tomahawk::source_ptr& from, const Tomahawk::source_ptr& to )
{
    if ( from.isNull() || to.isNull() )
        return;

    if ( to->isLocal() )
    {
#ifndef ENABLE_HEADLESS
        LatchedStatusItem* item = new LatchedStatusItem( from, to, this );
        m_jobs[ from->nodeId() ] = item;
        JobStatusView::instance()->model()->addJob( item );
#endif

        connect( from.data(), SIGNAL( offline() ), this, SLOT( sourceOffline() ), Qt::UniqueConnection );
    }
}

void
LatchedStatusManager::sourceOffline()
{
    Tomahawk::Source* s = qobject_cast< Tomahawk::Source* >( sender() );
    Q_ASSERT( s );

    if ( m_jobs.contains( s->nodeId() ) )
    {
        QPointer< LatchedStatusItem> job = m_jobs.take( s->nodeId() ).data();
        if ( !job.isNull() )
            job.data()->stop();
    }
}


void
LatchedStatusManager::latchedOff( const Tomahawk::source_ptr& from, const Tomahawk::source_ptr& to )
{
    if ( from.isNull() || to.isNull() )
        return;

    if ( to->isLocal() && m_jobs.contains( from->nodeId() ) )
    {
        QPointer< LatchedStatusItem > item = m_jobs.take( from->nodeId() );
        if ( !item.isNull() )
            item.data()->stop();
    }
}


QPixmap
LatchedStatusManager::pixmap() const
{
    return TomahawkUtils::defaultPixmap( TomahawkUtils::HeadphonesOn, TomahawkUtils::Original, QSize( 128, 128 ) );
}
