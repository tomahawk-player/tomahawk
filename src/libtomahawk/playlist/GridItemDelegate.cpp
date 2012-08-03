/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011-2012, Leo Franchi            <lfranchi@kde.org>
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

#include "GridItemDelegate.h"

#include <QApplication>
#include <QPainter>
#include <QAbstractItemView>
#include <QMouseEvent>
#include <QTimeLine>

#include "Artist.h"
#include "Query.h"
#include "Result.h"
#include "Source.h"
#include "audio/AudioEngine.h"

#include "utils/TomahawkUtilsGui.h"
#include "utils/PixmapDelegateFader.h"
#include <utils/Closure.h>

#include "playlist/PlayableItem.h"
#include "playlist/PlayableProxyModel.h"
#include "GridView.h"
#include "ViewManager.h"
#include "utils/AnimatedSpinner.h"
#include "widgets/ImageButton.h"
#include "utils/Logger.h"

namespace {
    static const int FADE_DURATION = 200;
};


GridItemDelegate::GridItemDelegate( QAbstractItemView* parent, PlayableProxyModel* proxy )
    : QStyledItemDelegate( (QObject*)parent )
    , m_view( parent )
    , m_model( proxy )
{
    if ( m_view && m_view->metaObject()->indexOfSignal( "modelChanged()" ) > -1 )
        connect( m_view, SIGNAL( modelChanged() ), this, SLOT( modelChanged() ) );

    connect( proxy, SIGNAL( rowsAboutToBeInserted( QModelIndex, int, int ) ), SLOT( modelChanged() ) );
    connect( proxy, SIGNAL( rowsAboutToBeRemoved( QModelIndex, int, int ) ), SLOT( modelChanged() ) );
    connect( m_view, SIGNAL( scrolledContents( int, int ) ), SLOT( onViewChanged() ) );
    connect( m_view, SIGNAL( resized() ), SLOT( onViewChanged() ) );
}


QSize
GridItemDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    if ( m_itemSize.isNull() )
    {
        QSize size = QStyledItemDelegate::sizeHint( option, index );
        return size;
    }
    else
        return m_itemSize;
}


void
GridItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    PlayableItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
    if ( !item )
        return;

    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, QModelIndex() );
    qApp->style()->drawControl( QStyle::CE_ItemViewItem, &opt, painter );

    painter->save();
    painter->setRenderHint( QPainter::Antialiasing );

    QRect r = option.rect;
    QString top, bottom;
    if ( !item->album().isNull() )
    {
        top = item->album()->name();

        if ( !item->album()->artist().isNull() )
            bottom = item->album()->artist()->name();
    }
    else if ( !item->artist().isNull() )
    {
        top = item->artist()->name();
    }
    else
    {
        top = item->query()->track();
        bottom = item->query()->artist();
    }

    if ( !m_covers.contains( index ) )
    {
        if ( !item->album().isNull() )
        {
            m_covers.insert( index, QSharedPointer< Tomahawk::PixmapDelegateFader >( new Tomahawk::PixmapDelegateFader( item->album(), r.size(), TomahawkUtils::Grid ) ) );
        }
        else if ( !item->artist().isNull() )
        {
            m_covers.insert( index, QSharedPointer< Tomahawk::PixmapDelegateFader >( new Tomahawk::PixmapDelegateFader( item->artist(), r.size(), TomahawkUtils::Grid ) ) );
        }
        else
        {
            m_covers.insert( index, QSharedPointer< Tomahawk::PixmapDelegateFader >( new Tomahawk::PixmapDelegateFader( item->query(), r.size(), TomahawkUtils::Grid ) ) );
        }

        NewClosure( m_covers[ index ], SIGNAL( repaintRequest() ),
                    const_cast<GridItemDelegate*>(this), SLOT( doUpdateIndex( QPersistentModelIndex ) ), QPersistentModelIndex( index ) )->setAutoDelete( false );
    }

    QSharedPointer< Tomahawk::PixmapDelegateFader > fader = m_covers[ index ];
    if ( fader->size() != r.size() )
        fader->setSize( r.size() );

    const QPixmap cover = fader->currentPixmap();
    painter->drawPixmap( r, cover );

    qreal opacity = -1.;
    if ( m_hoverFaders.contains( index ) )
    {
        const qreal pct = ( m_hoverFaders[ index ]->currentFrame() / 100. );
        opacity = 0.35 - pct * 0.35;
    }
    else if ( m_hoverIndex == index )
    {
        opacity = 0.35;
    }

    if ( opacity > -1. )
    {
        painter->save();

        painter->setPen( QColor( 33, 33, 33 ) );
        painter->setBrush( QColor( 33, 33, 33 ) );
        painter->setOpacity( opacity );
        painter->drawRect( r );

        painter->restore();
    }

    painter->save();

    painter->setPen( Qt::black );
    painter->setBrush( Qt::black );
    painter->setOpacity( 0.5 );
    painter->drawRoundedRect( r.adjusted( 4, +r.height() - 36, -4, -4 ), 3, 3 );

    painter->restore();

    painter->setPen( opt.palette.color( QPalette::HighlightedText ) );
    QTextOption to;
    to.setWrapMode( QTextOption::NoWrap );

    QString text;
    QFont font = opt.font;
    font.setPixelSize( 10 );
    QFont boldFont = font;
    boldFont.setBold( true );
    boldFont.setPixelSize( 14 );

    QRect textRect = option.rect.adjusted( 6, option.rect.height() - 36, -4, -6 );
    painter->setFont( font );
    int bottomHeight = painter->fontMetrics().boundingRect( bottom ).height();
    painter->setFont( boldFont );
    int topHeight = painter->fontMetrics().boundingRect( top ).height();

    bool oneLiner = false;
    if ( bottom.isEmpty() )
        oneLiner = true;
    else
        oneLiner = ( textRect.height() < topHeight + bottomHeight );

    if ( oneLiner )
    {
        to.setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
        text = painter->fontMetrics().elidedText( top, Qt::ElideRight, textRect.width() - 3 );
        painter->drawText( textRect, text, to );
    }
    else
    {
        to.setAlignment( Qt::AlignHCenter | Qt::AlignTop );
        text = painter->fontMetrics().elidedText( top, Qt::ElideRight, textRect.width() - 3 );
        painter->drawText( textRect, text, to );

        painter->setFont( font );
        // If the user is hovering over an artist rect, draw a background so she knows it's clickable
        QRect r = textRect;
        r.setTop( r.bottom() - painter->fontMetrics().height() );
        r.adjust( 4, 0, -4, -1 );
        if ( m_hoveringOver == index )
        {
            TomahawkUtils::drawQueryBackground( painter, opt.palette, r, 1.1 );
            painter->setPen( opt.palette.color( QPalette::HighlightedText ) );
        }

        to.setAlignment( Qt::AlignHCenter | Qt::AlignBottom );
        text = painter->fontMetrics().elidedText( bottom, Qt::ElideRight, textRect.width() - 10 );
        painter->drawText( textRect.adjusted( 5, -1, -5, -1 ), text, to );

        // Calculate rect of artist on-hover button click area
        m_artistNameRects[ index ] = r;
    }

    painter->restore();
}


void
GridItemDelegate::onPlayClicked( const QPersistentModelIndex& index )
{
    QPoint pos = m_playButton[ index ]->pos();
    foreach ( ImageButton* button, m_playButton )
        button->deleteLater();
    m_playButton.clear();

    AnimatedSpinner* spinner = new AnimatedSpinner( m_view );
    spinner->setAutoCenter( false );
    spinner->fadeIn();
    spinner->move( pos );
    spinner->setFocusPolicy( Qt::NoFocus );
    spinner->installEventFilter( this );

    m_spinner[ index ] = spinner;

    PlayableItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
    if ( item )
    {
        NewClosure( AudioEngine::instance(), SIGNAL( loading( Tomahawk::result_ptr ) ),
                    const_cast<GridItemDelegate*>(this), SLOT( onPlaybackStarted( QPersistentModelIndex ) ), QPersistentModelIndex( index ) );

        m_closures.remove( index );

        m_closures.insertMulti( index, NewClosure( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ),
                                                   const_cast<GridItemDelegate*>(this), SLOT( onPlaylistChanged( QPersistentModelIndex ) ), QPersistentModelIndex( index ) ) );
        m_closures.insertMulti( index, NewClosure( AudioEngine::instance(), SIGNAL( stopped() ),
                                       const_cast<GridItemDelegate*>(this), SLOT( onPlaylistChanged( QPersistentModelIndex ) ), QPersistentModelIndex( index ) ) );

        foreach ( _detail::Closure* closure, m_closures.values( index ) )
            closure->setAutoDelete( false );

        connect( AudioEngine::instance(), SIGNAL( stopped() ), SLOT( onPlaybackFinished() ) );

        if ( !item->query().isNull() )
            AudioEngine::instance()->playItem( Tomahawk::playlistinterface_ptr(), item->query() );
        else if ( !item->album().isNull() )
            AudioEngine::instance()->playItem( item->album() );
        else if ( !item->artist().isNull() )
            AudioEngine::instance()->playItem( item->artist() );
    }
}


bool
GridItemDelegate::editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{
    Q_UNUSED( model );
    Q_UNUSED( option );

    if ( event->type() != QEvent::MouseButtonRelease &&
         event->type() != QEvent::MouseMove &&
         event->type() != QEvent::MouseButtonPress &&
         event->type() != QEvent::Leave )
        return false;

    bool hoveringArtist = false;
    if ( m_artistNameRects.contains( index ) )
    {
        const QRect artistNameRect = m_artistNameRects[ index ];
        const QMouseEvent* ev = static_cast< QMouseEvent* >( event );
        hoveringArtist = artistNameRect.contains( ev->pos() );
    }

    if ( event->type() == QEvent::MouseMove )
    {
        if ( hoveringArtist )
            m_view->setCursor( Qt::PointingHandCursor );
        else
            m_view->setCursor( Qt::ArrowCursor );

        foreach ( const QModelIndex& idx, m_playButton.keys() )
        {
            if ( index != idx )
                m_playButton.take( idx )->deleteLater();
        }

        if ( !m_playButton.contains( index ) && !m_spinner.contains( index ) && !m_pauseButton.contains( index ) )
        {
            foreach ( ImageButton* button, m_playButton )
                button->deleteLater();
            m_playButton.clear();

            ImageButton* button = new ImageButton( m_view );
            button->setPixmap( RESPATH "images/play-rest.png" );
            button->setPixmap( RESPATH "images/play-pressed.png", QIcon::Off, QIcon::Active );
            button->setFixedSize( 48, 48 );
            button->move( option.rect.center() - QPoint( 23, 23 ) );
            button->setContentsMargins( 0, 0, 0, 0 );
            button->setFocusPolicy( Qt::NoFocus );
            button->installEventFilter( this );
            button->show();

            NewClosure( button, SIGNAL( clicked( bool ) ),
                        const_cast<GridItemDelegate*>(this), SLOT( onPlayClicked( QPersistentModelIndex ) ), QPersistentModelIndex( index ) );

            m_playButton[ index ] = button;
        }

        if ( m_hoveringOver != index || ( !hoveringArtist && m_hoveringOver.isValid() ) )
        {
            emit updateIndex( m_hoveringOver );

            if ( hoveringArtist )
                m_hoveringOver = index;
            else
                m_hoveringOver = QPersistentModelIndex();

            emit updateIndex( index );
        }

        if ( m_hoverIndex != index )
        {
            if ( m_hoverIndex.isValid() )
            {
                QTimeLine* fadeOut = createTimeline( QTimeLine::Forward );
                _detail::Closure* c = NewClosure( fadeOut, SIGNAL( frameChanged( int ) ), this, SLOT( fadingFrameChanged( QPersistentModelIndex ) ), QPersistentModelIndex( m_hoverIndex ) );
                c->setAutoDelete( false );
                c = NewClosure( fadeOut, SIGNAL( finished() ), this, SLOT( fadingFrameFinished( QPersistentModelIndex ) ), QPersistentModelIndex( m_hoverIndex ) );
                c->setAutoDelete( false );
                m_hoverFaders[ m_hoverIndex ] = fadeOut;
                fadeOut->start();
            }

            emit updateIndex( m_hoverIndex );
            m_hoverIndex = index;

            QTimeLine* fadeIn = createTimeline( QTimeLine::Backward );
            _detail::Closure* c = NewClosure( fadeIn, SIGNAL( frameChanged( int ) ), this, SLOT( fadingFrameChanged( QPersistentModelIndex ) ), QPersistentModelIndex( index ) );
            c->setAutoDelete( false );
            c = NewClosure( fadeIn, SIGNAL( finished() ), this, SLOT( fadingFrameFinished( QPersistentModelIndex ) ), QPersistentModelIndex( index ) );
            c->setAutoDelete( false );

            m_hoverFaders[ index ] = fadeIn;
            fadeIn->start();

            emit updateIndex( index );
        }

        event->accept();
        return true;
    }

    // reset mouse cursor. we switch to a pointing hand cursor when hovering an artist name
    m_view->setCursor( Qt::ArrowCursor );

    if ( hoveringArtist )
    {

        if ( event->type() == QEvent::MouseButtonRelease )
        {
            PlayableItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
            if ( !item )
                return false;

            if ( !item->query().isNull() )
                ViewManager::instance()->show( Tomahawk::Artist::get( item->query()->artist() ) );
            else if ( !item->album().isNull() && !item->album()->artist().isNull() )
                ViewManager::instance()->show( item->album()->artist() );

            event->accept();
            return true;
        }
        else if ( event->type() == QEvent::MouseButtonPress )
        {
            // Stop the whole album from having a down click action as we just want the artist name to be clicked
            event->accept();
            return true;
        }
    }

    return false;
}


void
GridItemDelegate::modelChanged()
{
    m_artistNameRects.clear();
    m_hoveringOver = QPersistentModelIndex();
    m_hoverIndex = QPersistentModelIndex();

    foreach ( ImageButton* button, m_playButton )
        button->deleteLater();
    m_playButton.clear();
    foreach ( ImageButton* button, m_pauseButton )
        button->deleteLater();
    m_pauseButton.clear();
    foreach ( QWidget* widget, m_spinner )
        widget->deleteLater();
    m_spinner.clear();

    if ( GridView* view = qobject_cast< GridView* >( m_view ) )
        m_model = view->proxyModel();
}


void
GridItemDelegate::doUpdateIndex( const QPersistentModelIndex& idx )
{
    if ( !idx.isValid() )
        return;
    emit updateIndex( idx );
}


void
GridItemDelegate::onViewChanged()
{
    foreach ( const QPersistentModelIndex& index, m_spinner.keys() )
    {
        QRect rect = m_view->visualRect( index );
        m_spinner.value( index )->move( rect.center() - QPoint( 23, 23 ) );
    }
    foreach ( const QPersistentModelIndex& index, m_playButton.keys() )
    {
        QRect rect = m_view->visualRect( index );
        m_playButton.value( index )->move( rect.center() - QPoint( 23, 23 ) );
    }
    foreach ( const QPersistentModelIndex& index, m_pauseButton.keys() )
    {
        QRect rect = m_view->visualRect( index );
        m_pauseButton.value( index )->move( rect.center() - QPoint( 23, 23 ) );
    }
}


void
GridItemDelegate::onPlaybackFinished()
{
    foreach ( ImageButton* button, m_pauseButton )
        button->deleteLater();
    m_pauseButton.clear();

    emit stoppedPlaying( QModelIndex() );
}


void
GridItemDelegate::onPlaylistChanged( const QPersistentModelIndex& index )
{
    PlayableItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
    if ( item )
    {
        bool finished = false;

        if ( !item->query().isNull() )
        {
            if ( !item->query()->numResults() || AudioEngine::instance()->currentTrack() != item->query()->results().first() )
                finished = true;
        }
        else if ( !item->album().isNull() )
        {
            if ( AudioEngine::instance()->currentTrackPlaylist() != item->album()->playlistInterface( Tomahawk::Mixed ) )
                finished = true;
        }
        else if ( !item->artist().isNull() )
        {
            if ( AudioEngine::instance()->currentTrackPlaylist() != item->artist()->playlistInterface( Tomahawk::Mixed ) )
                finished = true;
        }

        if ( finished )
        {
            foreach ( _detail::Closure* closure, m_closures.values( index ) )
                closure->deleteLater();

            if ( m_pauseButton.contains( index ) )
            {
                m_pauseButton[ index ]->deleteLater();
                m_pauseButton.remove( index );
            }
            if ( m_spinner.contains( index ) )
            {
                m_spinner[ index ]->deleteLater();
                m_spinner.remove( index );
            }

            emit stoppedPlaying( index );
        }
    }
}


void
GridItemDelegate::onPlaybackStarted( const QPersistentModelIndex& index )
{
    if ( !m_spinner.contains( index ) )
        return;

    QPoint pos = m_spinner[ index ]->pos();
    foreach ( QWidget* widget, m_spinner.values() )
    {
        delete widget;
    }
    m_spinner.clear();

    ImageButton* button = new ImageButton( m_view );
    button->setPixmap( RESPATH "images/pause-rest.png" );
    button->setPixmap( RESPATH "images/pause-pressed.png", QIcon::Off, QIcon::Active );
    button->setFixedSize( 48, 48 );
    button->move( pos );
    button->setContentsMargins( 0, 0, 0, 0 );
    button->setFocusPolicy( Qt::NoFocus );
    button->installEventFilter( this );
    button->show();

    connect( button, SIGNAL( clicked( bool ) ), AudioEngine::instance(), SLOT( playPause() ) );

    m_pauseButton[ index ] = button;

    emit startedPlaying( index );
}


void
GridItemDelegate::fadingFrameChanged( const QPersistentModelIndex& idx )
{
    emit updateIndex( idx );
}


void
GridItemDelegate::fadingFrameFinished( const QPersistentModelIndex& idx )
{
    if ( m_hoverFaders.contains( idx ) )
    {
        m_hoverFaders.take( idx )->deleteLater();
        emit updateIndex( idx );
    }
}


QTimeLine*
GridItemDelegate::createTimeline( QTimeLine::Direction direction )
{
    QTimeLine* timeline = new QTimeLine( FADE_DURATION, this );
    timeline->setDirection( direction );
    timeline->setCurveShape( QTimeLine::LinearCurve );
    timeline->setUpdateInterval( 30 );
    timeline->setStartFrame( 0 );
    timeline->setEndFrame( 100 );

    return timeline;
}


bool
GridItemDelegate::eventFilter( QObject* obj, QEvent* event )
{
    if ( event->type() == QEvent::Wheel )
    {
        QWheelEvent* we = static_cast<QWheelEvent*>( event );
        QWheelEvent* wheelEvent = new QWheelEvent(
            we->pos(),
            we->globalPos(),
            we->delta(),
            we->buttons(),
            we->modifiers(),
            we->orientation() );

        qApp->postEvent( m_view->viewport(), wheelEvent );
        event->accept();
        return true;
    }
    else
        return QObject::eventFilter( obj, event );
}
