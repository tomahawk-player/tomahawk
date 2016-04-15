/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2016, Dominik Schmidt <domme@tomahawk-player.org>
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

#include "DownloadButton.h"

#include "Artist.h"
#include "Album.h"
#include "Result.h"
#include "DownloadManager.h"
#include "utils/TomahawkStyle.h"
#include "utils/WebPopup.h"
#include "utils/Logger.h"

#include <QPainter>
#include <QEvent>
#include <QAbstractItemView>
#include <QDesktopServices>

using namespace Tomahawk;


DownloadButton::DownloadButton( const Tomahawk::query_ptr& query, QWidget* parent, QAbstractItemView* view, const QModelIndex& index )
    : DropDownButton( parent )
    , m_view( view )
    , m_index( index )
{
    init();

    setQuery( query );
}


DownloadButton::DownloadButton( QWidget* parent )
    : DropDownButton( parent )
{
    init();
}


void
DownloadButton::init()
{
    connect( this, SIGNAL( clicked() ), this, SLOT( addDownloadJob() ) );
    connect( this, SIGNAL( activated( int ) ), this, SLOT( addDownloadJob() ) );
}


DownloadButton::~DownloadButton()
{
}


void
DownloadButton::setQuery( const query_ptr& query )
{
    if ( !m_query.isNull() )
    {
        m_query->disconnect( this );
    }
    if ( !m_result.isNull() )
    {
        m_result->disconnect( this );
    }

    clear();
    m_result.clear();

    m_query = query;

    if ( query.isNull() )
        return;

    Tomahawk::result_ptr result = query->numResults( true ) ? query->results().first() : Tomahawk::result_ptr();
    if ( result.isNull() )
        return;

    QStringList formats;
    foreach ( const DownloadFormat& format, result->downloadFormats() )
    {
        formats << QObject::tr( "Download %1" ).arg( format.extension.toUpper() );
    }

    addItems( formats );
}

void
DownloadButton::addDownloadJob()
{
    if ( m_query.isNull() )
        return;

    Tomahawk::result_ptr result = m_query->numResults( true ) ? m_query->results().first() : Tomahawk::result_ptr();
    if ( result.isNull() )
        return;

    if ( handleClickPreDownload( m_query ) )
        return;

    if ( !result->downloadFormats().isEmpty() )
    {
        if ( m_view && m_index.isValid() )
        {
            m_view->closePersistentEditor( m_index );
        }
        else
        {
            m_result = result;
            connect( result.data(), SIGNAL( updated() ), SLOT( update() ) );
            connect( m_query.data(), SIGNAL( resultsChanged() ), SLOT( update() ) );
        }

        DownloadManager::instance()->addJob( result->toDownloadJob( result->downloadFormats().at( currentIndex() ) ) );
    }
    else
    {
        handleClickPostDownload( m_query );
    }
}


void
DownloadButton::paintEvent( QPaintEvent* event )
{
    QPainter p( this );
    setupPainter( &p );

    if ( DownloadButton::drawPrimitive( &p, contentsRect(), m_query, m_hovering ) )
        setCursor( Qt::PointingHandCursor );
    else
        setCursor( Qt::ArrowCursor );
}


bool
DownloadButton::drawPrimitive( QPainter* painter, const QRect& rect, const Tomahawk::query_ptr& query, bool hovering )
{
    if ( query.isNull() )
        return false;

    Tomahawk::result_ptr result = query->numResults( true ) ? query->results().first() : Tomahawk::result_ptr();
    if ( result.isNull() )
        return false;

    if ( result->downloadJob() && result->downloadJob()->state() != DownloadJob::Finished )
    {
        // if downloadJob exists and is not finished, paint a progress bar
        painter->save();
        painter->setPen( TomahawkStyle::PLAYLIST_PROGRESS_FOREGROUND.darker() );
        painter->setBrush( TomahawkStyle::PLAYLIST_PROGRESS_BACKGROUND );
        painter->drawRect( rect.adjusted( 2, 2, -2, -2 ) );
        painter->setPen( TomahawkStyle::PLAYLIST_PROGRESS_FOREGROUND );
        painter->setBrush( TomahawkStyle::PLAYLIST_PROGRESS_FOREGROUND );
        QRect fillp = rect.adjusted( 3, 3, -3, -3 );
        fillp.setWidth( float(fillp.width()) * ( float( result->downloadJob()->progressPercentage() ) / 100.0 ) );
        painter->drawRect( fillp );
        painter->restore();
    }
    else
    {
        QString text;
        bool itemsAvailable = false;
        if ( result &&
           ( ( !result->downloadFormats().isEmpty() && !DownloadManager::instance()->localFileForDownload( result->downloadFormats().first().url.toString() ).isEmpty() ) ||
             ( result->downloadJob() && result->downloadJob()->state() == DownloadJob::Finished ) ) )
        {
            text = QObject::tr( "View in Finder" );
        }
        else if ( !result->downloadFormats().isEmpty() )
        {
            text = tr( "Download %1" ).arg( query->results().first()->downloadFormats().first().extension.toUpper() );
            itemsAvailable = true;
        }
        else if ( !result->purchaseUrl().isEmpty() )
        {
            text = tr( "Buy" );
        }

        if ( !text.isEmpty() )
            DropDownButton::drawPrimitive( painter, rect, text, hovering, itemsAvailable );
        else
        {
            // this result can neither be bought nor downloaded
            return false;
        }
    }

    return true;
}

bool
DownloadButton::handleEditorEvent(QEvent* event , QAbstractItemView* view, PlayableProxyModel* model, const QModelIndex& index)
{
    if ( event->type() == QEvent::MouseButtonRelease )
    {
        PlayableItem* item = model->sourceModel()->itemFromIndex( model->mapToSource( index ) );
        if ( !item  && ! item->query() )
            return false;

        if ( handleClickPreDownload( item->query() ) )
            return true;

        if( item->query()->numResults( true ) && !item->query()->results().first()->downloadFormats().isEmpty() )
        {
            model->sourceModel()->setAllColumnsEditable( true );
            view->edit( index );
            model->sourceModel()->setAllColumnsEditable( false );
            return true;
        }

        return handleClickPostDownload( item->query() );
    }

    return false;
}


bool
DownloadButton::handleClickPreDownload( const Tomahawk::query_ptr& query )
{
    // view in folder
    if ( !DownloadManager::instance()->localUrlForDownload( query ).isEmpty() )
    {
        QDesktopServices::openUrl( DownloadManager::instance()->localUrlForDownload( query ) );
        return true;
    }

    // download in progress
    Tomahawk::result_ptr result = query->numResults( true ) ? query->results().first() : Tomahawk::result_ptr();
    if ( result && result->downloadJob() && result->downloadJob()->state() != DownloadJob::Finished )
    {
        // do nothing, handled
        return true;
    }

    return false;
}


bool
DownloadButton::handleClickPostDownload( const Tomahawk::query_ptr& query )
{
    // handle buy click
    Tomahawk::result_ptr result = query->numResults( true ) ? query->results().first() : Tomahawk::result_ptr();
    if ( result && !result->purchaseUrl().isEmpty() )
    {
        WebPopup* popup = new WebPopup( result->purchaseUrl(), QSize( 400, 800 ) );
        connect( result.data(), SIGNAL( destroyed() ), popup, SLOT( close() ) );
        return true;
    }

    return false;
}


QWidget*
DownloadButton::handleCreateEditor( QWidget* parent, const query_ptr& query, QAbstractItemView* view, const QModelIndex& index )
{
    Tomahawk::result_ptr result = query->numResults( true ) ? query->results().first() : Tomahawk::result_ptr();
    if ( result && !result->downloadFormats().isEmpty() && !result->downloadJob() )
    {
        return new DownloadButton( query, parent, view, index );
    }

    return 0;
}

