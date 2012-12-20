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

#include "ContextWidget.h"
#include "ui_ContextWidget.h"

#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QPropertyAnimation>
#include <QTimeLine>

#include "context/ContextPage.h"
#include "context/pages/RelatedArtistsContext.h"
#include "context/pages/TopTracksContext.h"
#include "context/pages/WikipediaContext.h"

#include "Source.h"

#include "utils/StyleHelper.h"
#include "utils/TomahawkUtilsGui.h"

#define ANIMATION_TIME 450
#define SLIDE_TIME 350

using namespace Tomahawk;


ContextWidget::ContextWidget( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::ContextWidget )
    , m_currentView( 0 )
    , m_visible( false )
{
    ui->setupUi( this );
    TomahawkUtils::unmarginLayout( layout() );
    setContentsMargins( 0, 0, 0, 0 );

    m_scene = new QGraphicsScene( this );

    TopTracksContext* ttc = new TopTracksContext();
    RelatedArtistsContext* rac = new RelatedArtistsContext();
    WebContext* wiki = new WikipediaContext();
    /*WebContext* lastfm = new LastfmContext();*/

    m_views << ttc;
    m_views << rac;
    m_views << wiki;
/*    m_views << lastfm;*/

    foreach ( ContextPage* view, m_views )
    {
        ContextProxyPage* page = new ContextProxyPage();
        page->setPage( view );
        m_scene->addItem( page );

        connect( page, SIGNAL( focused() ), SLOT( onPageFocused() ) );
        m_pages << page;
    }

    ui->contextView->setScene( m_scene );
    ui->contextView->setFrameShape( QFrame::NoFrame );
    ui->contextView->setStyleSheet( "QGraphicsView { background: transparent; }" );
    ui->contextView->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    ui->contextView->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    ui->contextView->hide();

    QPalette whitePal = ui->toggleButton->palette();
    whitePal.setColor( QPalette::Foreground, Qt::white );
    ui->toggleButton->setPalette( whitePal );
    ui->toggleButton->setCursor( Qt::PointingHandCursor );

    m_minHeight = TomahawkUtils::defaultFontHeight() * 1.4;
    ui->toggleButton->setMinimumHeight( m_minHeight );

    setAutoFillBackground( true );
    setFixedHeight( m_minHeight );

    ensurePolished();
    QPalette pal = palette();
    pal.setBrush( QPalette::Window, QColor( "#272b2e" ) );
    setPalette( pal );

    connect( ui->toggleButton, SIGNAL( clicked() ), SLOT( toggleSize() ) );

    m_timeLine = new QTimeLine( ANIMATION_TIME, this );
    m_timeLine->setUpdateInterval( 20 );
    m_timeLine->setEasingCurve( QEasingCurve::OutCubic );

    connect( m_timeLine, SIGNAL( frameChanged( int ) ), SLOT( onAnimationStep( int ) ) );
    connect( m_timeLine, SIGNAL( finished() ), SLOT( onAnimationFinished() ) );
}


ContextWidget::~ContextWidget()
{
}


void
ContextWidget::changeEvent( QEvent* e )
{
    QWidget::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}


void
ContextWidget::layoutViews( bool animate )
{
    int smallViewWidth = 120;
    float smallViewOpacity = 0.6;

    int margin = 6;
    int maxVisible = 2;
    int itemSize = ( m_scene->sceneRect().width() - smallViewWidth * 2 ) / maxVisible;
    int firstPos = margin;
    float opacity;

    if ( m_currentView > 0 )
        firstPos = smallViewWidth;

    if ( m_currentView + maxVisible >= m_pages.count() )
    {
        int delta = m_pages.count() - m_currentView;
        firstPos = m_scene->sceneRect().width() - ( delta * itemSize ) + 1;
    }

    for ( int i = 0; i < m_pages.count(); i++ )
    {
        QGraphicsWidget* view = m_pages.at( i );

        int x = firstPos - ( ( m_currentView - i ) * itemSize );

        if ( ( x < smallViewWidth && x < firstPos ) || i > m_currentView + maxVisible - 1 )
        {
            opacity = smallViewOpacity;
        }
        else
        {
            opacity = 1.0;
        }

        {
            QPropertyAnimation* animation = new QPropertyAnimation( view, "opacity" );
            animation->setDuration( SLIDE_TIME );
            animation->setEndValue( opacity );
            animation->start();
        }

        QRect rect( x, margin, itemSize - margin * 2, m_scene->sceneRect().height() - margin * 2 );
        if ( animate )
        {
            {
                QPropertyAnimation* animation = new QPropertyAnimation( view, "geometry" );
                animation->setDuration( SLIDE_TIME );
                animation->setEndValue( rect );
                animation->start();
            }
        }
        else
        {
            view->setGeometry( rect );
        }
    }
}


void
ContextWidget::onPageFocused()
{
    ContextProxyPage* widget = qobject_cast< ContextProxyPage* >( sender() );

    int i = 0;
    foreach ( ContextProxyPage* view, m_pages )
    {
        if ( view == widget )
        {
            m_currentView = i;
            layoutViews( true );
            return;
        }

        i++;
    }
}


void
ContextWidget::fadeOut( bool animate )
{
    foreach ( QGraphicsWidget* view, m_pages )
    {
        if ( animate )
        {
            QPropertyAnimation* animation = new QPropertyAnimation( view, "opacity" );
            animation->setDuration( SLIDE_TIME );
            animation->setEndValue( 0.0 );
            animation->start();
        }
        else
            view->setOpacity( 0.0 );
    }
}


void
ContextWidget::setArtist( const Tomahawk::artist_ptr& artist )
{
    if ( artist.isNull() )
        return;

    m_artist = artist;
    if ( height() > m_minHeight )
    {
        foreach ( ContextProxyPage* proxy, m_pages )
        {
            proxy->page()->setArtist( artist );
        }

        layoutViews( true );
    }
}


void
ContextWidget::setAlbum( const Tomahawk::album_ptr& album )
{
    if ( album.isNull() )
        return;

    m_album = album;
    if ( height() > m_minHeight )
    {
        foreach ( ContextProxyPage* proxy, m_pages )
        {
            proxy->page()->setAlbum( album );
        }

        layoutViews( true );
    }
}


void
ContextWidget::setQuery( const Tomahawk::query_ptr& query, bool force )
{
    if ( query.isNull() )
        return;
    if ( !force && !m_query.isNull() && query->artist() == m_query->artist() )
        return;

    m_query = query;
    if ( height() > m_minHeight )
    {
        foreach ( ContextProxyPage* proxy, m_pages )
        {
            proxy->page()->setQuery( query );
        }

        layoutViews( true );
    }
}


void
ContextWidget::toggleSize()
{
    m_maxHeight = TomahawkUtils::tomahawkWindow()->height() * 0.3;

    if ( height() == m_minHeight )
    {
        m_timeLine->setFrameRange( height(), m_maxHeight );
        m_timeLine->setDirection( QTimeLine::Forward );
        m_timeLine->start();
    }
    else
    {
        m_visible = false;
        ui->contextView->hide();

        m_timeLine->setFrameRange( m_minHeight, height() );
        m_timeLine->setDirection( QTimeLine::Backward );
        m_timeLine->start();
    }
}


void
ContextWidget::onAnimationStep( int frame )
{
    setFixedHeight( frame );
}


void
ContextWidget::onAnimationFinished()
{
    if ( m_timeLine->direction() == QTimeLine::Forward )
    {
        setFixedHeight( m_maxHeight );
        m_visible = true;
        ui->contextView->show();

        fadeOut( false );
        m_scene->setSceneRect( ui->contextView->viewport()->rect() );
        layoutViews( false );
        setArtist( m_artist );
        setAlbum( m_album );
        setQuery( m_query, true );

        ui->toggleButton->setText( tr( "Hide Footnotes" ) );
    }
    else
    {
        setFixedHeight( m_minHeight );

        ui->toggleButton->setText( tr( "Show Footnotes" ) );
    }
}


void
ContextWidget::paintEvent( QPaintEvent* e )
{
    QWidget::paintEvent( e );
}


void
ContextWidget::resizeEvent( QResizeEvent* e )
{
    QWidget::resizeEvent( e );

    if ( m_visible )
    {
        m_scene->setSceneRect( ui->contextView->viewport()->rect() );
        layoutViews( false );
    }
}
