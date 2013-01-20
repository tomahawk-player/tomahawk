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

#include <QApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>

#include "Artist.h"
#include "Album.h"
#include "Query.h"
#include "ContextMenu.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "ViewManager.h"
#include "Source.h"

#define BOXMARGIN 2
#define DASH "  -  "

using namespace Tomahawk;


QueryLabel::QueryLabel( QWidget* parent, Qt::WindowFlags flags )
    : QFrame( parent, flags )
    , m_type( Complete )
{
    init();
}


QueryLabel::QueryLabel( DisplayType type, QWidget* parent, Qt::WindowFlags flags )
    : QFrame( parent, flags )
    , m_type( type )
{
    init();
}


QueryLabel::QueryLabel( const Tomahawk::result_ptr& result, DisplayType type, QWidget* parent, Qt::WindowFlags flags )
    : QFrame( parent, flags )
    , m_type( type )
    , m_result( result )
{
    init();
}


QueryLabel::QueryLabel( const Tomahawk::query_ptr& query, DisplayType type, QWidget* parent, Qt::WindowFlags flags )
    : QFrame( parent, flags )
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

    m_hoverType = None;
    setContentsMargins( 0, 0, 0, 0 );
    setMouseTracking( true );

    m_align = Qt::AlignLeft | Qt::AlignVCenter;
    m_mode = Qt::ElideMiddle;

    m_jumpLinkVisible = false;
}


QString
QueryLabel::text() const
{
    QString text;

    if ( m_result.isNull() && m_query.isNull() && m_artist.isNull() && m_album.isNull() )
        return m_text;

    if ( !m_result.isNull() )
    {
        if ( m_type & Artist )
        {
            text += m_result->artist()->name();
        }
        if ( m_type & Album && !m_result->album()->name().isEmpty() )
        {
            smartAppend( text, m_result->album()->name() );
        }
        if ( m_type & Track )
        {
            smartAppend( text, m_result->track() );
        }
    }
    else if ( !m_query.isNull() )
    {
        if ( m_type & Artist )
        {
            text += m_query->artist();
        }
        if ( m_type & Album && !m_query->album().isEmpty() )
        {
            smartAppend( text, m_query->album() );
        }
        if ( m_type & Track )
        {
            smartAppend( text, m_query->track() );
        }
    }
    else if ( !m_artist.isNull() )
    {
        text += m_artist->name();
    }
    else if ( !m_album.isNull() )
    {
        text += m_album->name();
    }

    return text;
}


QString
QueryLabel::track() const
{
    if ( m_result.isNull() && m_query.isNull() )
        return QString();

    if ( !m_result.isNull() )
        return m_result->track();
    else
        return m_query->track();
}


void
QueryLabel::setText( const QString& text )
{
    setContentsMargins( m_textMargins );

    m_result.clear();
    m_query.clear();
    m_artist.clear();
    m_album.clear();
    m_text = text;

    updateLabel();

    emit textChanged( m_text );
    emit resultChanged( m_result );
}


void
QueryLabel::onResultChanged()
{
    m_query = m_result->toQuery();
    m_artist = m_result->artist();
    m_album = m_result->album();

    updateLabel();

    emit textChanged( text() );
}


void
QueryLabel::setResult( const Tomahawk::result_ptr& result )
{
    if ( result.isNull() )
        return;

    if ( !m_text.isEmpty() && contentsMargins().left() != 0 ) // FIXME: hacky
        m_textMargins = contentsMargins();

    setContentsMargins( BOXMARGIN * 2, BOXMARGIN / 2, BOXMARGIN * 2, BOXMARGIN / 2);

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

    setContentsMargins( BOXMARGIN * 2, BOXMARGIN / 2, BOXMARGIN * 2, BOXMARGIN / 2 );

    if ( m_query.isNull() || m_query.data() != query.data() )
    {
        m_query = query;
        m_artist = Artist::get( query->artist() );
        m_album = Album::get( m_artist, query->album() );
        m_result.clear();

        updateLabel();

        emit textChanged( text() );
        emit queryChanged( m_query );
    }
}


void
QueryLabel::setArtist( const artist_ptr& artist )
{
    m_artist = artist;

    updateLabel();
    emit textChanged( text() );
}


void
QueryLabel::setAlbum( const album_ptr& album )
{
    m_album = album;

    updateLabel();
    emit textChanged( text() );
}


void
QueryLabel::setJumpLinkVisible( bool visible )
{
    m_jumpLinkVisible = visible;
    repaint();
}


Qt::Alignment
QueryLabel::alignment() const
{
    return m_align;
}


void
QueryLabel::setAlignment( Qt::Alignment alignment )
{
    if ( m_align != alignment )
    {
        m_align = alignment;
        update(); // no geometry change, repaint is sufficient
    }
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
        updateLabel();
    }
}


void
QueryLabel::updateLabel()
{
    m_hoverArea = QRect();
    m_hoverType = None;

    updateGeometry();
    update();
}


void
QueryLabel::setExtraContentsMargins( int left, int top, int right, int bottom )
{
    QMargins margins = contentsMargins();
    margins.setLeft( margins.left() + left );
    margins.setTop( margins.top() + top );
    margins.setRight( margins.right() + right );
    margins.setBottom( margins.bottom() + bottom );
    setContentsMargins( margins );
}


QSize
QueryLabel::sizeHint() const
{
    const QFontMetrics& fm = fontMetrics();
    QSize size( fm.width( text() ) + contentsMargins().left() * 2, fm.height() + contentsMargins().top() * 2 );
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
            QSize size( fm.width( "..." ), fm.height() + contentsMargins().top() * 2  );
            return size;
        }
    }
}


void
QueryLabel::paintEvent( QPaintEvent* event )
{
    QFrame::paintEvent( event );
    QPainter p( this );
    QRect r = contentsRect();
    QString s = text();
    const QFontMetrics& fm = fontMetrics();
    const QString elidedText = fm.elidedText( s, m_mode, r.width() );

    p.save();
    p.setRenderHint( QPainter::Antialiasing );

    if ( m_hoverArea.width() )
    {
        if ( elidedText != s )
        {
            m_hoverArea.setLeft( 0 );
            m_hoverArea.setRight( fm.width( elidedText ) + contentsMargins().left() * 2 );
            m_hoverType = Track;
        }

        TomahawkUtils::drawQueryBackground( &p, m_hoverArea );
    }

    if ( elidedText != s || ( m_result.isNull() && m_query.isNull() && m_artist.isNull() && m_album.isNull() ) )
    {
        if ( m_hoverArea.width() )
        {
            p.setBrush( TomahawkUtils::Colors::SELECTION_BACKGROUND );
            p.setPen( TomahawkUtils::Colors::SELECTION_FOREGROUND );
        }
        else
        {
            p.setBrush( palette().window() );
            p.setPen( palette().color( foregroundRole() ) );
        }

        p.drawText( r, m_align, elidedText );
    }
    else
    {
        int dashX = fm.width( DASH );
        int artistX = m_type & Artist ? fm.width( artist()->name() ) : 0;
        int albumX = m_type & Album ? fm.width( album()->name() ) : 0;
        int trackX = m_type & Track ? fm.width( track() ) : 0;

        if ( m_type & Artist )
        {
            p.setBrush( palette().window() );
            p.setPen( palette().color( foregroundRole() ) );

            if ( m_hoverType == Artist )
            {
                p.setBrush( TomahawkUtils::Colors::SELECTION_BACKGROUND );
                p.setPen( TomahawkUtils::Colors::SELECTION_FOREGROUND );
            }

            p.drawText( r, m_align, artist()->name() );
            r.adjust( artistX, 0, 0, 0 );
        }
        if ( m_type & Album && !album()->name().isEmpty() )
        {
            p.setBrush( palette().window() );
            p.setPen( palette().color( foregroundRole() ) );

            if ( m_type & Artist )
            {
                p.drawText( r, m_align, DASH );
                r.adjust( dashX, 0, 0, 0 );
            }
            if ( m_hoverType == Album )
            {
                p.setBrush( TomahawkUtils::Colors::SELECTION_BACKGROUND );
                p.setPen( TomahawkUtils::Colors::SELECTION_FOREGROUND );
            }

            p.drawText( r, m_align, album()->name() );
            r.adjust( albumX, 0, 0, 0 );
        }
        if ( m_type & Track )
        {
            p.setBrush( palette().window() );
            p.setPen( palette().color( foregroundRole() ) );

            if ( m_type & Artist || ( m_type & Album && !album()->name().isEmpty() ) )
            {
                p.drawText( r, m_align, DASH );
                r.adjust( dashX, 0, 0, 0 );
            }
            if ( m_hoverType == Track )
            {
                p.setBrush( TomahawkUtils::Colors::SELECTION_BACKGROUND );
                p.setPen( TomahawkUtils::Colors::SELECTION_FOREGROUND );
            }

            p.drawText( r, m_align, track() );
            r.adjust( trackX, 0, 0, 0 );
        }

        if ( m_jumpLinkVisible )
        {
            r.adjust( 6, 0, 0, 0 );
            r.setWidth( r.height() );
            p.drawPixmap( r, TomahawkUtils::defaultPixmap( TomahawkUtils::JumpLink, TomahawkUtils::Original, r.size() ) );
        }
    }

    p.restore();
}


void
QueryLabel::changeEvent( QEvent* event )
{
    QFrame::changeEvent( event );
    switch ( event->type() )
    {
        case QEvent::FontChange:
        case QEvent::ApplicationFontChange:
            updateLabel();
            break;

        default:
            break;
    }
}


void
QueryLabel::contextMenuEvent( QContextMenuEvent* event )
{
    m_contextMenu->clear();

    switch( m_hoverType )
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
    QFrame::mousePressEvent( event );
    m_time.restart();
    m_dragPos = event->pos();
}


void
QueryLabel::mouseReleaseEvent( QMouseEvent* event )
{
    QFrame::mouseReleaseEvent( event );

    m_dragPos = QPoint();
    if ( m_time.elapsed() < qApp->doubleClickInterval() )
    {
        switch( m_hoverType )
        {
            case Artist:
                emit clickedArtist();
                break;
            case Album:
                emit clickedAlbum();
                break;
            case Track:
                emit clickedTrack();
                break;

            case Complete:
                ViewManager::instance()->showCurrentTrack();
                break;

            default:
                emit clicked();
        }
    }
}


void
QueryLabel::mouseMoveEvent( QMouseEvent* event )
{
    QFrame::mouseMoveEvent( event );
    int x = event->x();

    if ( event->buttons() & Qt::LeftButton &&
       ( m_dragPos - event->pos() ).manhattanLength() >= QApplication::startDragDistance() )
    {
        startDrag();
        leaveEvent( 0 );
        return;
    }

    if ( m_query.isNull() && m_result.isNull() && m_artist.isNull() && m_album.isNull() )
    {
        m_hoverArea = QRect();
        m_hoverType = None;
        return;
    }

    QFontMetrics fm = fontMetrics();

    int dashX = fm.width( DASH );
    int artistX = m_type & Artist ? fm.width( artist()->name() ) : 0;
    int albumX = m_type & Album ? fm.width( album()->name() ) : 0;
    int trackX = m_type & Track ? fm.width( track() ) : 0;

    if ( m_type & Track )
    {
        trackX += contentsMargins().left();
    }
    if ( m_type & Album && !album()->name().isEmpty() )
    {
        trackX += albumX + dashX;
        albumX += contentsMargins().left();
    }
    if ( m_type & Artist )
    {
        albumX += artistX + dashX;
        trackX += artistX + dashX;
        artistX += contentsMargins().left();
    }

    QRect hoverArea;
    m_hoverType = None;

    if ( m_align & Qt::AlignLeft )
    {
        if ( m_type & Artist && x < artistX )
        {
            m_hoverType = Artist;
            hoverArea.setLeft( 0 );
            hoverArea.setRight( artistX + contentsMargins().left() - 1 );
        }
        else if ( m_type & Album && !album()->name().isEmpty() && x < albumX && x > artistX )
        {
            m_hoverType = Album;
            int spacing = ( m_type & Artist ) ? dashX : 0;
            hoverArea.setLeft( artistX + spacing - contentsMargins().left() );
            hoverArea.setRight( albumX + contentsMargins().left() - 1 );
        }
        else if ( m_type & Track && x < trackX && x > albumX )
        {
            m_hoverType = Track;
            int spacing = ( m_type & Album && !album()->name().isEmpty() ) ? dashX : 0;
            hoverArea.setLeft( albumX + spacing );
            hoverArea.setRight( trackX + contentsMargins().left() - 1 );
        }
        else if ( m_jumpLinkVisible && x < trackX + 6 + fm.height() && x > trackX + 6 )
        {
            m_hoverType = Complete;
        }
    }
    else
    {
        hoverArea.setLeft( 0 );
        hoverArea.setRight( width() - 1 );

        if ( m_type & Artist )
            m_hoverType = Artist;
        else if ( m_type & Album )
            m_hoverType = Album;
        else if ( m_type & Track )
            m_hoverType = Track;
    }

    if ( hoverArea.width() )
    {
        hoverArea.setY( 1 );
        hoverArea.setHeight( height() - 2 );
    }

    if ( m_hoverType != None )
        setCursor( Qt::PointingHandCursor );
    else
        setCursor( Qt::ArrowCursor );

    if ( hoverArea != m_hoverArea )
    {
        m_hoverArea = hoverArea;
        repaint();
    }
}


void
QueryLabel::leaveEvent( QEvent* event )
{
    Q_UNUSED( event );
    m_hoverArea = QRect();
    m_hoverType = None;
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

    switch( m_hoverType )
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


QString
QueryLabel::smartAppend( QString& text, const QString& appendage ) const
{
    QString s;
    if ( !text.isEmpty() )
        s = DASH;

    text += s + appendage;
    return text;
}
