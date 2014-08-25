/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "QueryLabel.h"

#include "Artist.h"
#include "Album.h"
#include "Query.h"
#include "ContextMenu.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "ViewManager.h"
#include "Source.h"

#include <QApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QDrag>
#include <QMimeData>

using namespace Tomahawk;


QueryLabel::QueryLabel( QWidget* parent, Qt::WindowFlags flags )
    : QLabel( parent, flags )
    , m_type( None )
{
    init();
}


QueryLabel::QueryLabel( DisplayType type, QWidget* parent, Qt::WindowFlags flags )
    : QLabel( parent, flags )
    , m_type( type )
{
    init();
}


QueryLabel::QueryLabel( const Tomahawk::result_ptr& result, DisplayType type, QWidget* parent, Qt::WindowFlags flags )
    : QLabel( parent, flags )
    , m_type( type )
    , m_result( result )
{
    init();
}


QueryLabel::QueryLabel( const Tomahawk::query_ptr& query, DisplayType type, QWidget* parent, Qt::WindowFlags flags )
    : QLabel( parent, flags )
    , m_type( type )
    , m_query( query )
{
    init();
}


QueryLabel::~QueryLabel()
{
}


void
QueryLabel::init()
{
    m_contextMenu = new ContextMenu( this );
    m_contextMenu->setSupportedActions( ContextMenu::ActionQueue | ContextMenu::ActionCopyLink | ContextMenu::ActionStopAfter | ContextMenu::ActionLove | ContextMenu::ActionPage );

    setMouseTracking( true );

    setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    m_mode = Qt::ElideMiddle;
    m_hovering = false;
}


QString
QueryLabel::text() const
{
    if ( m_result.isNull() && m_query.isNull() && m_artist.isNull() && m_album.isNull() )
        return m_text;

    if ( !m_result.isNull() )
    {
        if ( m_type & Artist )
        {
            return m_result->track()->artist();
        }
        if ( m_type & Album && !m_result->track()->album().isEmpty() )
        {
            return m_result->track()->album();
        }
        if ( m_type & Track )
        {
            return m_result->track()->track();
        }
    }
    else if ( !m_query.isNull() )
    {
        if ( m_type & Artist )
        {
            return m_query->track()->artist();
        }
        if ( m_type & Album && !m_query->track()->album().isEmpty() )
        {
            return m_query->track()->album();
        }
        if ( m_type & Track )
        {
            return m_query->track()->track();
        }
    }
    else if ( m_type & Artist && !m_artist.isNull() )
    {
        return m_artist->name();
    }
    else if ( m_type & Album && !m_album.isNull() )
    {
        return m_album->name();
    }

    return QString();
}


void
QueryLabel::setText( const QString& text )
{
    clear();
    QLabel::setText( m_text );
    m_text = text;

    updateGeometry();
    update();

    emit textChanged( m_text );
    emit resultChanged( m_result );
}


void
QueryLabel::clear()
{
    m_text.clear();
    m_result.clear();
    m_query.clear();
    m_artist.clear();
    m_album.clear();

    QLabel::clear();
}


void
QueryLabel::onResultChanged()
{
    m_query = m_result->toQuery();
    m_artist = m_result->track()->artistPtr();
    m_album = m_result->track()->albumPtr();

    updateGeometry();
    update();

    emit textChanged( text() );
}


void
QueryLabel::setResult( const Tomahawk::result_ptr& result )
{
    if ( result.isNull() )
        return;

    if ( m_result.isNull() || m_result.data() != result.data() )
    {
        m_result = result;
        connect( m_result.data(), SIGNAL( updated() ), SLOT( onResultChanged() ) );

        onResultChanged();
        emit resultChanged( m_result );
    }
}


void
QueryLabel::setQuery( const Tomahawk::query_ptr& query )
{
    if ( query.isNull() )
        return;

    if ( m_query.isNull() || m_query.data() != query.data() )
    {
        m_query = query;
        m_artist = Artist::get( query->track()->artist() );
        m_album = Album::get( m_artist, query->track()->album() );
        m_result.clear();

        updateGeometry();
        update();

        emit textChanged( text() );
        emit queryChanged( m_query );
    }
}


void
QueryLabel::setArtist( const artist_ptr& artist )
{
    m_artist = artist;

    updateGeometry();
    update();
    emit textChanged( text() );
}


void
QueryLabel::setAlbum( const album_ptr& album )
{
    m_album = album;

    updateGeometry();
    update();
    emit textChanged( text() );
}



Qt::TextElideMode
QueryLabel::elideMode() const
{
    return m_mode;
}


void
QueryLabel::setElideMode( Qt::TextElideMode mode )
{
    if ( m_mode != mode )
    {
        m_mode = mode;
        updateGeometry();
        update();
    }
}


QSize
QueryLabel::sizeHint() const
{
    const QFontMetrics& fm = fontMetrics();
    QSize size( fm.width( text() ) + contentsMargins().left() + contentsMargins().right(),
                fm.height() + contentsMargins().top() + contentsMargins().bottom() );
    return size;
}


QSize
QueryLabel::minimumSizeHint() const
{
    switch ( m_mode )
    {
        case Qt::ElideNone:
            return sizeHint();

        default:
        {
            const QFontMetrics& fm = fontMetrics();
            QSize size( fm.width( "..." ), fm.height() + contentsMargins().top() + contentsMargins().bottom() );
            return size;
        }
    }
}


void
QueryLabel::paintEvent( QPaintEvent* /* event */ )
{
    QPainter p( this );
    p.setRenderHint( QPainter::TextAntialiasing );
    QRect r = contentsRect();

    if ( m_hovering )
    {
        QFont f = p.font();
        f.setUnderline( true );
        p.setFont( f );
    }

    const QFontMetrics fm( p.font() );
    p.drawText( r, alignment(), fm.elidedText( text(), m_mode, r.width() ) );
}


void
QueryLabel::changeEvent( QEvent* event )
{
    QLabel::changeEvent( event );
    switch ( event->type() )
    {
        case QEvent::FontChange:
        case QEvent::ApplicationFontChange:
            updateGeometry();
            update();
            break;

        default:
            break;
    }
}


void
QueryLabel::contextMenuEvent( QContextMenuEvent* event )
{
    m_contextMenu->clear();

    switch( m_type )
    {
        case Artist:
        {
            m_contextMenu->setArtist( artist() );
            break;
        }
        case Album:
        {
            m_contextMenu->setAlbum( album() );
            break;
        }

        default:
            m_contextMenu->setQuery( m_query );
    }

    m_contextMenu->exec( event->globalPos() );
}


void
QueryLabel::mousePressEvent( QMouseEvent* event )
{
    QLabel::mousePressEvent( event );
    m_time.restart();
    m_dragPos = event->pos();
}


void
QueryLabel::mouseReleaseEvent( QMouseEvent* event )
{
    QLabel::mouseReleaseEvent( event );

    m_dragPos = QPoint();
    if ( m_time.elapsed() < qApp->doubleClickInterval() )
    {
        switch ( m_type )
        {
            case Artist:
            {
                ViewManager::instance()->show( artist() );
                break;
            }
            case Album:
            {
                ViewManager::instance()->show( album() );
                break;
            }

            default:
                emit clicked();
//                ViewManager::instance()->show( m_query );
        }
    }
}


void
QueryLabel::mouseMoveEvent( QMouseEvent* event )
{
    QLabel::mouseMoveEvent( event );

    if ( event->buttons() & Qt::LeftButton &&
       ( m_dragPos - event->pos() ).manhattanLength() >= QApplication::startDragDistance() )
    {
        startDrag();
        leaveEvent( 0 );
        return;
    }

    if ( !m_hovering )
    {
        m_hovering = true;
        setCursor( Qt::PointingHandCursor );
        repaint();
    }
}


void
QueryLabel::leaveEvent( QEvent* event )
{
    QLabel::leaveEvent( event );

    m_hovering = false;
    repaint();
}


void
QueryLabel::startDrag()
{
    if ( m_query.isNull() && m_album.isNull() && m_artist.isNull() )
        return;

    QDrag *drag = new QDrag( this );
    QByteArray data;
    QDataStream dataStream( &data, QIODevice::WriteOnly );
    QMimeData* mimeData = new QMimeData();
    mimeData->setText( text() );

    switch( m_type )
    {
            case Artist:
            {
                dataStream << artist()->name();
                mimeData->setData( "application/tomahawk.metadata.artist", data );
                drag->setPixmap( TomahawkUtils::createDragPixmap( TomahawkUtils::MediaTypeArtist ) );
                break;
            }
            case Album:
            {
                dataStream << artist()->name();
                dataStream << album()->name();
                mimeData->setData( "application/tomahawk.metadata.album", data );
                drag->setPixmap( TomahawkUtils::createDragPixmap( TomahawkUtils::MediaTypeAlbum ) );
                break;
            }

            default:
            {
                dataStream << qlonglong( &m_query );
                mimeData->setData( "application/tomahawk.query.list", data );
                drag->setPixmap( TomahawkUtils::createDragPixmap( TomahawkUtils::MediaTypeTrack ) );
                break;
            }
    }

    drag->setMimeData( mimeData );

//    QPoint hotSpot = event->pos() - child->pos();
//    drag->setHotSpot( hotSpot );

    drag->exec( Qt::CopyAction );
}


void
QueryLabel::setType( DisplayType type )
{
    m_type = type;
    updateGeometry();
    update();
}
