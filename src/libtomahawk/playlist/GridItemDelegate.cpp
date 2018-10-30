/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "Artist.h"
#include "Query.h"
#include "Result.h"
#include "Source.h"
#include "GridView.h"
#include "ViewManager.h"
#include "audio/AudioEngine.h"
#include "playlist/PlayableItem.h"
#include "playlist/PlayableProxyModel.h"
#include "widgets/HoverControls.h"
#include "widgets/DownloadButton.h"
#include "widgets/ImageButton.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/PixmapDelegateFader.h"
#include "utils/Closure.h"
#include "utils/AnimatedSpinner.h"
#include "utils/DpiScaler.h"
#include "utils/Logger.h"

#include <QApplication>
#include <QPainter>
#include <QAbstractItemView>
#include <QMouseEvent>
#include <QTimeLine>

namespace {
    static const int FADE_DURATION = 400;
};


GridItemDelegate::GridItemDelegate( QAbstractItemView* parent, PlayableProxyModel* proxy )
    : QStyledItemDelegate( (QObject*)parent )
    , m_view( parent )
    , m_model( proxy )
    , m_itemWidth( 0 )
    , m_showPosition( false )
    , m_showBuyButtons( false )
    , m_wordWrapping( false )
    , m_margin( TomahawkUtils::DpiScaler::scaledY( parent, 32 ) )
{
    if ( m_view && m_view->metaObject()->indexOfSignal( "modelChanged()" ) > -1 )
        connect( m_view, SIGNAL( modelChanged() ), this, SLOT( modelChanged() ) );

    m_font = m_view->font();
    m_smallFont = m_font;
    m_font.setPointSize( TomahawkUtils::defaultFontSize() + 2 );
    m_smallFont.setPointSize( TomahawkUtils::defaultFontSize() );

    connect( this, SIGNAL( updateIndex( QModelIndex ) ), parent, SLOT( update( QModelIndex ) ) );

    connect( proxy, SIGNAL( rowsAboutToBeInserted( QModelIndex, int, int ) ), SLOT( modelChanged() ) );
    connect( proxy, SIGNAL( rowsAboutToBeRemoved( QModelIndex, int, int ) ), SLOT( modelChanged() ) );
    connect( proxy->playlistInterface().data(), SIGNAL( currentIndexChanged() ), SLOT( onCurrentIndexChanged() ), Qt::UniqueConnection );

    connect( m_view, SIGNAL( scrolledContents( int, int ) ), SLOT( onViewChanged() ) );
    connect( m_view, SIGNAL( resized() ), SLOT( onViewChanged() ) );
}


QSize
GridItemDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    if ( m_itemWidth == 0 )
    {
        QSize size = QStyledItemDelegate::sizeHint( option, index );
        return size;
    }
    else
    {
        const QFontMetrics fm( m_font );
        const QFontMetrics fms( m_smallFont );

        if ( !m_wordWrapping )
            return QSize( m_itemWidth, m_itemWidth + fm.height() + m_margin * 0.8 );

        const int buyButtonHeight = m_showBuyButtons ? 40 : 0;
        return QSize( m_itemWidth, m_itemWidth + fm.height() + fms.height() + buyButtonHeight + m_margin * 0.8 );
    }
}


QSize
GridItemDelegate::itemSize() const
{
    return sizeHint( QStyleOptionViewItem(), m_model->index( 0, 0 ) );
}


void
GridItemDelegate::setShowPosition( bool enabled )
{
    m_showPosition = enabled;
}


void
GridItemDelegate::setWordWrapping( bool enabled )
{
    m_wordWrapping = enabled;
}


void
GridItemDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    PlayableItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
    if ( !item || !index.isValid() )
        return;

    QStyleOptionViewItem opt = option;
    initStyleOption( &opt, QModelIndex() );
//    qApp->style()->drawControl( QStyle::CE_ItemViewItem, &opt, painter );

    QRect r = option.rect;
    r.setHeight( r.width() );

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
    else if ( !item->query().isNull() )
    {
        top = item->query()->track()->track();
        bottom = item->query()->track()->artist();
    }
    else
    {
        return;
    }

    painter->save();
    painter->setRenderHint( QPainter::TextAntialiasing );

    if ( !m_covers.contains( index ) )
    {
        if ( !item->album().isNull() )
        {
            m_covers.insert( index, QSharedPointer< Tomahawk::PixmapDelegateFader >( new Tomahawk::PixmapDelegateFader( item->album(), r.size(), TomahawkUtils::Original, false ) ) );
        }
        else if ( !item->artist().isNull() )
        {
            m_covers.insert( index, QSharedPointer< Tomahawk::PixmapDelegateFader >( new Tomahawk::PixmapDelegateFader( item->artist(), r.size(), TomahawkUtils::Original, false ) ) );
        }
        else
        {
            m_covers.insert( index, QSharedPointer< Tomahawk::PixmapDelegateFader >( new Tomahawk::PixmapDelegateFader( item->query(), r.size(), TomahawkUtils::Original, false ) ) );
        }

        NewClosure( m_covers[ index ], SIGNAL( repaintRequest() ),
                    const_cast<GridItemDelegate*>(this), SLOT( doUpdateIndex( QPersistentModelIndex ) ), QPersistentModelIndex( index ) )->setAutoDelete( false );
    }

    QSharedPointer< Tomahawk::PixmapDelegateFader > fader = m_covers[ index ];
    if ( fader->size() != r.size() )
        fader->setSize( r.size() );
    const QPixmap cover = fader->currentPixmap();

    qreal opacity = -1.0;
    qreal pct = -1.0;
    if ( m_hoverFaders.contains( index ) )
    {
        pct = ( m_hoverFaders[ index ]->currentFrame() / 100.0 );
        opacity = 1.0 - pct * 0.70;
    }
    else if ( m_hoverIndex == index )
    {
        opacity = 0.3;
        pct = 1.0;
    }
    if ( opacity > -1.0 )
    {
        painter->save();

        const int cropIn = pct * ( (qreal)cover.width() * 0.10 );
        const QRect crop = cover.rect().adjusted( cropIn, cropIn, -cropIn, -cropIn );

        painter->drawPixmap( r, cover.copy( crop ).scaled( r.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );

        painter->setOpacity( 1.0 - opacity );
        painter->setPen( Qt::transparent );
        painter->setBrush( Qt::black );
        painter->drawRect( r );

        painter->restore();
    }
    else
    {
        painter->drawPixmap( r, cover );
    }

    QTextOption to;
    to.setWrapMode( QTextOption::NoWrap );

    QString text;
    QRect textRect = option.rect.adjusted( 0, r.height() + m_margin / 4, 0, -m_margin / 2 + m_margin / 8 );
    bool oneLiner = false;
    if ( bottom.isEmpty() )
        oneLiner = true;

    painter->setPen( TomahawkStyle::SELECTION_FOREGROUND );
    painter->setFont( m_font );
    painter->setPen( Qt::black );
    painter->setOpacity( 0.8 );

    if ( m_showPosition )
    {
        painter->save();

        if ( !oneLiner )
        {
            QFont figFont = m_font;
            figFont.setPixelSize( textRect.height() - m_margin / 8 );
            painter->setFont( figFont );
        }

        const QString fig = QString::number( index.row() + 1 );
        painter->drawText( textRect, fig, QTextOption( Qt::AlignLeft | Qt::AlignVCenter ) );

        textRect.adjust( painter->fontMetrics().boundingRect( textRect, Qt::AlignLeft | Qt::AlignVCenter, fig ).width() + m_margin / 4, 0, 0, 0 );
        painter->restore();
    }

    if ( oneLiner )
    {
        // If the user is hovering over an artist rect, draw a background so they knows it's clickable
        if ( m_hoveringOverArtist == index )
        {
            QFont f = painter->font();
            f.setUnderline( true );
            painter->setFont( f );
        }

        to.setAlignment( Qt::AlignLeft | Qt::AlignTop );
        text = painter->fontMetrics().elidedText( top, Qt::ElideRight, textRect.width() - m_margin / 4 );
        painter->drawText( textRect, text, to );

        // Calculate rect of artist on-hover button click area
        m_artistNameRects[ index ] = painter->fontMetrics().boundingRect( textRect, Qt::AlignLeft | Qt::AlignTop, text );
    }
    else
    {
        painter->save();
        // If the user is hovering over an album rect, underline the album name
        if ( m_hoveringOverAlbum == index )
        {
            QFont f = painter->font();
            f.setUnderline( true );
            painter->setFont( f );
        }

        if ( m_showBuyButtons && !item->query().isNull() )
        {
            textRect.adjust( 0, 0, 0, -40 );
        }

        to.setAlignment( Qt::AlignLeft | Qt::AlignTop );
        text = painter->fontMetrics().elidedText( top, Qt::ElideRight, textRect.width() - m_margin / 4 );
        painter->drawText( textRect, text, to );

        if ( item->album() )
        {
            // Calculate rect of album on-hover button click area
            m_albumNameRects[ index ] = painter->fontMetrics().boundingRect( textRect, Qt::AlignLeft | Qt::AlignTop, text );
        }
        painter->restore();

        painter->save();
        painter->setOpacity( 0.6 );
        painter->setFont( m_smallFont );

        // If the user is hovering over an artist rect, underline the artist name
        if ( m_hoveringOverArtist == index )
        {
            QFont f = painter->font();
            f.setUnderline( true );
            painter->setFont( f );
        }

        textRect.adjust( 0, painter->fontMetrics().height() + m_margin / 16, 0, 0 );
        to.setAlignment( Qt::AlignLeft | Qt::AlignBottom );
        text = painter->fontMetrics().elidedText( bottom, Qt::ElideRight, textRect.width() - m_margin / 4 );
        painter->drawText( textRect, text, to );

        // Calculate rect of artist on-hover button click area
        m_artistNameRects[ index ] = painter->fontMetrics().boundingRect( textRect, Qt::AlignLeft | Qt::AlignBottom, text );
        painter->restore();

        if ( m_showBuyButtons && !item->query().isNull() )
        {
            QRect r = textRect;
            r.setY( textRect.y() + textRect.height() + 8 );
            r.setHeight( 32 );

            if( DownloadButton::drawPrimitive(painter, r, item->query(), m_hoveringOverBuyButton == index ) )
            {
                m_buyButtonRects[ index ] = r;
            }
        }
    }

    painter->restore();
}


void
GridItemDelegate::onPlayClicked( const QPersistentModelIndex& index )
{
    clearButtons();

    AnimatedSpinner* spinner = new AnimatedSpinner( m_view );
    spinner->setAutoCenter( false );
    spinner->fadeIn();

    QRect r = m_view->visualRect( index );
    r.setHeight( r.width() );
    QPoint pos = r.center() - QPoint( ( spinner->width() ) / 2 - 1,
                                      ( spinner->height() ) / 2 - 1 );

    spinner->move( pos );
    spinner->setFocusPolicy( Qt::NoFocus );
    spinner->installEventFilter( this );

    m_spinner[ index ] = spinner;

    PlayableItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );

    NewClosure( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ),
                const_cast<GridItemDelegate*>(this), SLOT( onPlaybackStarted( QPersistentModelIndex ) ), QPersistentModelIndex( index ) );

    if ( item )
    {
        if ( !item->query().isNull() )
            AudioEngine::instance()->playItem( m_model->playlistInterface(), item->query() );
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

    const QMouseEvent* ev = static_cast< QMouseEvent* >( event );
    bool hoveringArtist = false;
    bool hoveringAlbum = false;
    bool hoveringBuyButton = false;
    if ( m_artistNameRects.contains( index ) )
    {
        const QRect artistNameRect = m_artistNameRects[ index ];
        hoveringArtist = artistNameRect.contains( ev->pos() );
    }
    if ( m_albumNameRects.contains( index ) )
    {
        const QRect albumNameRect = m_albumNameRects[ index ];
        hoveringAlbum = albumNameRect.contains( ev->pos() );
    }
    if ( m_buyButtonRects.contains( index ) )
    {
        const QRect buyButtonRect = m_buyButtonRects[ index ];
        hoveringBuyButton = buyButtonRect.contains( ev->pos() );
    }

    QRect coverRect = m_view->visualRect( index );
    coverRect.setHeight( coverRect.width() );
    const bool hoveringCover = coverRect.contains( ev->pos() );

    if ( event->type() == QEvent::MouseMove )
    {
        if ( hoveringArtist || hoveringAlbum || hoveringBuyButton )
            m_view->setCursor( Qt::PointingHandCursor );
        else
            m_view->setCursor( Qt::ArrowCursor );

        foreach ( const QModelIndex& idx, m_hoverControls.keys() )
        {
            if ( index != idx )
                m_hoverControls.take( idx )->deleteLater();
        }

        if ( hoveringCover && !m_hoverControls.contains( index ) && !m_spinner.contains( index ) )
        {
            foreach ( HoverControls* control, m_hoverControls )
                control->deleteLater();
            m_hoverControls.clear();

            QRect cRect = option.rect;
            cRect.setHeight( cRect.width() );

            HoverControls* controls = new HoverControls( m_view );
            controls->setFixedSize( m_margin * 2, m_margin + m_margin / 4 );
            controls->move( cRect.center() - QPoint( controls->width() / 2 -1, controls->height() / 2 -1 ) );
            controls->setContentsMargins( 0, 0, 0, 0 );
            controls->setFocusPolicy( Qt::NoFocus );
            controls->installEventFilter( this );
            controls->show();

            NewClosure( controls, SIGNAL( play() ),
                        const_cast<GridItemDelegate*>(this), SLOT( onPlayClicked( QPersistentModelIndex ) ), QPersistentModelIndex( index ) );

            m_hoverControls[ index ] = controls;
        }

        if ( m_hoveringOverArtist != index || ( !hoveringArtist && m_hoveringOverArtist.isValid() ) )
        {
            emit updateIndex( m_hoveringOverArtist );

            if ( hoveringArtist )
                m_hoveringOverArtist = index;
            else
                m_hoveringOverArtist = QPersistentModelIndex();

            emit updateIndex( index );
        }
        if ( m_hoveringOverAlbum != index || ( !hoveringAlbum && m_hoveringOverAlbum.isValid() ) )
        {
            emit updateIndex( m_hoveringOverAlbum );

            if ( hoveringAlbum )
                m_hoveringOverAlbum = index;
            else
                m_hoveringOverAlbum = QPersistentModelIndex();

            emit updateIndex( index );
        }
        if ( m_hoveringOverBuyButton != index || ( !hoveringBuyButton && m_hoveringOverBuyButton.isValid() ) )
        {
            emit updateIndex( m_hoveringOverBuyButton );

            if ( hoveringBuyButton )
                m_hoveringOverBuyButton = index;
            else
                m_hoveringOverBuyButton = QPersistentModelIndex();

            emit updateIndex( index );
        }

        if ( m_hoverIndex != index || !hoveringCover )
        {
            if ( m_hoverIndex.isValid() )
            {
                int startFrame = 100;
                if ( m_hoverFaders.contains( m_hoverIndex ) )
                {
                    QTimeLine* oldFader = m_hoverFaders.take( m_hoverIndex );
                    startFrame = oldFader->currentFrame();
                    oldFader->deleteLater();
                }

                QTimeLine* fadeOut = createTimeline( QTimeLine::Backward, startFrame );
                _detail::Closure* c = NewClosure( fadeOut, SIGNAL( frameChanged( int ) ), this, SLOT( fadingFrameChanged( QPersistentModelIndex ) ), QPersistentModelIndex( m_hoverIndex ) );
                c->setAutoDelete( false );
                c = NewClosure( fadeOut, SIGNAL( finished() ), this, SLOT( fadingFrameFinished( QPersistentModelIndex ) ), QPersistentModelIndex( m_hoverIndex ) );
                c->setAutoDelete( false );

                m_hoverFaders[ m_hoverIndex ] = fadeOut;
                fadeOut->start();
            }
            emit updateIndex( m_hoverIndex );

            foreach ( HoverControls* controls, m_hoverControls )
                controls->deleteLater();
            m_hoverControls.clear();
            m_hoverIndex = QPersistentModelIndex();
        }

        if ( hoveringCover && m_hoverIndex != index )
        {
            m_hoverIndex = index;
            int startFrame = 0;
            if ( m_hoverFaders.contains( index ) )
            {
                QTimeLine* oldFader = m_hoverFaders.take( index );
                startFrame = oldFader->currentFrame();
                oldFader->deleteLater();
            }

            QTimeLine* fadeIn = createTimeline( QTimeLine::Forward, startFrame );
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

            if ( item->query() )
                ViewManager::instance()->show( item->query()->track()->artistPtr() );
            else if ( item->album() && item->album()->artist() )
                ViewManager::instance()->show( item->album()->artist() );
            else if ( item->artist() )
                ViewManager::instance()->show( item->artist() );

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
    if ( hoveringAlbum )
    {
        if ( event->type() == QEvent::MouseButtonRelease )
        {
            PlayableItem* item = m_model->sourceModel()->itemFromIndex( m_model->mapToSource( index ) );
            if ( !item )
                return false;

            if ( item->query() )
                ViewManager::instance()->show( item->query()->track()->albumPtr() );
            else if ( item->album() )
                ViewManager::instance()->show( item->album() );

            event->accept();
            return true;
        }
        else if ( event->type() == QEvent::MouseButtonPress )
        {
            // Stop the whole album from having a down click action as we just want the album name to be clicked
            event->accept();
            return true;
        }
    }

    if ( hoveringBuyButton )
    {
        return DownloadButton::handleEditorEvent( event, m_view, m_model, index );
    }

    return false;
}


void
GridItemDelegate::modelChanged()
{
    m_artistNameRects.clear();
    m_albumNameRects.clear();
    m_hoveringOverArtist = QPersistentModelIndex();
    m_hoveringOverAlbum = QPersistentModelIndex();
    m_hoveringOverBuyButton = QPersistentModelIndex();
    m_hoverIndex = QPersistentModelIndex();

    clearButtons();

    if ( GridView* view = qobject_cast< GridView* >( m_view ) )
        m_model = view->proxyModel();

    connect( m_model->playlistInterface().data(), SIGNAL( currentIndexChanged() ), SLOT( onCurrentIndexChanged() ), Qt::UniqueConnection );
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
        rect.setHeight( rect.width() );

        QWidget* spinner = m_spinner.value( index );
        QPoint pos = rect.center() - QPoint( ( spinner->width() ) / 2 - 1,
                                             ( spinner->height() ) / 2 - 1 );

        spinner->move( pos );
    }
    foreach ( const QPersistentModelIndex& index, m_hoverControls.keys() )
    {
        QRect rect = m_view->visualRect( index );
        rect.setHeight( rect.width() );

        HoverControls* controls = m_hoverControls.value( index );
        controls->move( rect.center() - QPoint( controls->width() / 2 -1, controls->height() / 2 -1 ) );
    }
}


void
GridItemDelegate::onPlaybackFinished()
{
    clearButtons();

    emit stoppedPlaying( QModelIndex() );
}


void
GridItemDelegate::onPlaybackStarted( const QPersistentModelIndex& index )
{
    if ( m_spinner.contains( index ) )
    {
        LoadingSpinner* spinner = static_cast<LoadingSpinner*>(m_spinner[ index ]);
        spinner->fadeOut();
    }

    clearButtons();

    emit startedPlaying( index );
}


void
GridItemDelegate::clearButtons()
{
    foreach ( HoverControls* control, m_hoverControls )
        control->deleteLater();
    m_hoverControls.clear();

    foreach ( QWidget* widget, m_spinner )
        widget->deleteLater();
    m_spinner.clear();
}


void
GridItemDelegate::onCurrentIndexChanged()
{
    tDebug() << Q_FUNC_INFO << m_model-> currentIndex();
    if ( m_model->currentIndex().isValid() )
    {
        onPlaybackStarted( m_model->currentIndex() );
    }
    else
        onPlaybackFinished();
}


void
GridItemDelegate::resetHoverIndex()
{
    foreach ( HoverControls* controls, m_hoverControls )
        controls->deleteLater();
    m_hoverControls.clear();

    if ( m_hoverIndex.isValid() )
    {
        int startFrame = 100;
        if ( m_hoverFaders.contains( m_hoverIndex ) )
        {
            QTimeLine* oldFader = m_hoverFaders.take( m_hoverIndex );
            startFrame = oldFader->currentFrame();
            oldFader->deleteLater();
        }

        QTimeLine* fadeOut = createTimeline( QTimeLine::Backward, startFrame );
        _detail::Closure* c = NewClosure( fadeOut, SIGNAL( frameChanged( int ) ), this, SLOT( fadingFrameChanged( QPersistentModelIndex ) ), QPersistentModelIndex( m_hoverIndex ) );
        c->setAutoDelete( false );
        c = NewClosure( fadeOut, SIGNAL( finished() ), this, SLOT( fadingFrameFinished( QPersistentModelIndex ) ), QPersistentModelIndex( m_hoverIndex ) );
        c->setAutoDelete( false );

        m_hoverFaders[ m_hoverIndex ] = fadeOut;
        fadeOut->start();
    }
    emit updateIndex( m_hoverIndex );

    m_hoverIndex = QPersistentModelIndex();

    QModelIndex idx = m_hoveringOverArtist;
    m_hoveringOverArtist = QPersistentModelIndex();
    doUpdateIndex( idx );

    idx = m_hoveringOverAlbum;
    m_hoveringOverAlbum = QPersistentModelIndex();
    doUpdateIndex( idx );

    idx = m_hoveringOverBuyButton;
    m_hoveringOverBuyButton = QPersistentModelIndex();
    doUpdateIndex( idx );
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
GridItemDelegate::createTimeline( QTimeLine::Direction direction, int startFrame )
{
    qreal dur = (qreal)FADE_DURATION * ( 1.0 - (qreal)startFrame / 100.0 );
    if ( direction == QTimeLine::Backward )
    {
        dur = (qreal)FADE_DURATION * ( (qreal)startFrame / 100.0 );
    }

    QTimeLine* timeline = new QTimeLine( dur, this );
    timeline->setDirection( direction );
    timeline->setCurveShape( QTimeLine::LinearCurve );
    timeline->setUpdateInterval( 20 );

    if ( direction == QTimeLine::Forward )
    {
        timeline->setStartFrame( startFrame );
        timeline->setEndFrame( 100 );
    }
    else
    {
        timeline->setStartFrame( 0 );
        timeline->setEndFrame( startFrame );
    }

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


QWidget*
GridItemDelegate::createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    PlayableItem* item = m_model->itemFromIndex( m_model->mapToSource( index ) );
    Q_ASSERT( item );

    return DownloadButton::handleCreateEditor( parent, item->query(), m_view, index );
}


void
GridItemDelegate::updateEditorGeometry( QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyledItemDelegate::updateEditorGeometry( editor, option, index );

    DownloadButton* comboBox = static_cast<DownloadButton*>(editor);
    comboBox->resize( option.rect.size() - QSize( 8, 0 ) );
    comboBox->move( option.rect.x() + 4, option.rect.y() );

    if ( m_buyButtonRects.contains( index ) )
    {
        editor->setGeometry( m_buyButtonRects.value( index ) );
    }

    if ( !comboBox->property( "shownPopup" ).toBool() )
    {
        comboBox->showPopup();
        comboBox->setProperty( "shownPopup", true );
    }
}


void
GridItemDelegate::setModelData( QWidget* editor, QAbstractItemModel* model, const QModelIndex& index ) const
{
}
