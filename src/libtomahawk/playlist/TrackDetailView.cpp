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

#include "TrackDetailView.h"

#include "Album.h"
#include "Track.h"
#include "audio/AudioEngine.h"
#include "widgets/PlayableCover.h"
#include "widgets/QueryLabel.h"
#include "widgets/ClickableLabel.h"
#include "widgets/CaptionLabel.h"
#include "PlaylistInterface.h"
#include "utils/TomahawkStyle.h"
#include "utils/ImageRegistry.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Closure.h"
#include "utils/WebPopup.h"
#include "utils/Logger.h"

#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QDesktopServices>

using namespace Tomahawk;

TrackDetailView::TrackDetailView( QWidget* parent )
    : QWidget( parent )
    , DpiScaler( this )
    , m_buyButtonVisible( false )
{
    setFixedWidth( scaledX( 200 ) );
    setContentsMargins( 0, 0, 0, 0 );

    QPalette pal( palette() );
    pal.setColor( QPalette::Background, TomahawkStyle::PAGE_BACKGROUND );
    setPalette( pal );
    setAutoFillBackground( true );

    m_playableCover = new PlayableCover( this );
    m_playableCover->setShowText( false );
    m_playableCover->setShowControls( false );
    m_playableCover->setType( PlayableCover::Album );
    m_playableCover->setFixedSize( QSize( width(), width() ) );

    QFont f = font();
    m_nameLabel = new QueryLabel( this );
    f.setPointSize( TomahawkUtils::defaultFontSize() + 3 );
    m_nameLabel->setFont( f );

    m_dateLabel = new QLabel( this );
    f.setPointSize( TomahawkUtils::defaultFontSize() + 1 );
    m_dateLabel->setFont( f );
    m_dateLabel->setStyleSheet( "QLabel { color: rgba( 0, 0, 0, 70% ) }" );
    m_dateLabel->hide(); //TODO

    m_lovedIcon = new QLabel( this );
    m_lovedIcon->setFixedWidth( TomahawkUtils::defaultFontSize() + 2 );
    m_lovedLabel = new QLabel( this );
    f.setWeight( QFont::DemiBold );
    f.setPointSize( TomahawkUtils::defaultFontSize() + 1 );
    m_lovedLabel->setFont( f );
    m_lovedLabel->setStyleSheet( "QLabel { color: rgba( 0, 0, 0, 50% ) }" );
    m_lovedIcon->setPixmap( ImageRegistry::instance()->pixmap( RESPATH "images/love.svg", QSize( m_lovedIcon->width(), m_lovedIcon->width() ) ) );
    m_lovedLabel->setText( tr( "Marked as Favorite" ) );

    m_infoBox = new QWidget;
    QHBoxLayout* hboxl = new QHBoxLayout;
    TomahawkUtils::unmarginLayout( hboxl );
    hboxl->setSpacing( 8 );
    hboxl->setContentsMargins( 0, 32, 0, 0 );
    hboxl->addWidget( m_lovedIcon );
    hboxl->addWidget( m_lovedLabel );
    m_infoBox->setLayout( hboxl );
    m_infoBox->hide();

    f.setWeight( QFont::Normal );
    f.setPointSize( TomahawkUtils::defaultFontSize() - 1 );

    m_resultsBoxLabel = new CaptionLabel( this );
    m_resultsBoxLabel->setFont( f );
    m_resultsBoxLabel->setStyleSheet( "QLabel { color: rgba( 0, 0, 0, 50% ) }" );
    m_resultsBoxLabel->setText( tr( "Alternate Sources:" ) );
    m_resultsBoxLabel->setFixedWidth( width() - 4 );
//    m_resultsBoxLabel->setFixedHeight( m_resultsBoxLabel->sizeHint().height() * 0.8 );
    m_resultsBoxLabel->hide();

    m_resultsBox = new QWidget;
    QVBoxLayout* resultsLayout = new QVBoxLayout;
    TomahawkUtils::unmarginLayout( resultsLayout );
    resultsLayout->setSpacing( 8 );
    resultsLayout->setContentsMargins( 0, 0, 0, 0 );
    resultsLayout->setSizeConstraint( QLayout::SetMinAndMaxSize );
    m_resultsBox->setLayout( resultsLayout );
    m_resultsBox->hide();

    m_resultsScrollArea = new QScrollArea;
    m_resultsScrollArea->setWidgetResizable( true );
    m_resultsScrollArea->setWidget( m_resultsBox );
    m_resultsScrollArea->setFrameShape( QFrame::NoFrame );
    m_resultsScrollArea->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    m_resultsScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_resultsScrollArea->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
    TomahawkStyle::styleScrollBar( m_resultsScrollArea->verticalScrollBar() );
    m_resultsScrollArea->hide();

    m_buyButton = new QPushButton;
    m_buyButton->setStyleSheet( "QPushButton:hover { font-size: 12px; color: #ffffff; background: #000000; border-style: solid; border-radius: 0px; border-width: 2px; border-color: #2b2b2b; }"
                                "QPushButton { font-size: 12px; color: #ffffff; background-color: #000000; border-style: solid; border-radius: 0px; border-width: 0px; }" );
    m_buyButton->setMinimumHeight( 30 );
    m_buyButton->setText( tr( "Buy Album" ) );
    m_buyButton->setVisible( false );
    connect( m_buyButton, SIGNAL( clicked() ), SLOT( onBuyButtonClicked() ) );

    QVBoxLayout* layout = new QVBoxLayout;
    TomahawkUtils::unmarginLayout( layout );
    layout->addWidget( m_playableCover );
    layout->addSpacerItem( new QSpacerItem( 0, 8, QSizePolicy::Minimum, QSizePolicy::Fixed ) );
    layout->addWidget( m_nameLabel );
    layout->addSpacerItem( new QSpacerItem( 0, 4, QSizePolicy::Minimum, QSizePolicy::Fixed ) );
    layout->addWidget( m_buyButton );
    layout->addWidget( m_dateLabel );
    layout->addWidget( m_infoBox );
    layout->addSpacerItem( new QSpacerItem( 0, 32, QSizePolicy::Minimum, QSizePolicy::Fixed ) );
    layout->addWidget( m_resultsBoxLabel );
    layout->addSpacerItem( new QSpacerItem( 0, 8, QSizePolicy::Minimum, QSizePolicy::Fixed ) );
    layout->addWidget( m_resultsScrollArea );
    layout->addStretch();
    layout->setStretchFactor( m_resultsScrollArea, 1 );

    setLayout( layout );
    setQuery( query_ptr() );

    connect( DownloadManager::instance(), SIGNAL( stateChanged( DownloadManager::DownloadManagerState, DownloadManager::DownloadManagerState ) ),
             SLOT( onDownloadManagerStateChanged( DownloadManager::DownloadManagerState, DownloadManager::DownloadManagerState ) ) );
}


TrackDetailView::~TrackDetailView()
{
}


void
TrackDetailView::setPlaylistInterface( const Tomahawk::playlistinterface_ptr& playlistInterface )
{
    m_playlistInterface = playlistInterface;
}


void
TrackDetailView::setQuery( const Tomahawk::query_ptr& query )
{
    if ( m_query )
    {
        if ( m_query->track()->albumPtr() && !m_query->track()->albumPtr()->name().isEmpty() )
        {
            disconnect( m_query->track()->albumPtr().data(), SIGNAL( updated() ), this, SLOT( onAlbumUpdated() ) );
        }
        disconnect( m_query->track().data(), SIGNAL( updated() ), this, SLOT( onCoverUpdated() ) );
        disconnect( m_query->track().data(), SIGNAL( socialActionsLoaded() ), this, SLOT( onSocialActionsLoaded() ) );
        disconnect( m_query.data(), SIGNAL( resultsChanged() ), this, SLOT( onResultsChanged() ) );
    }

    m_query = query;
    m_playableCover->setQuery( query );
    onResultsChanged();
    setSocialActions();
    onCoverUpdated();
    onAlbumUpdated();

    if ( !query )
    {
        m_dateLabel->setText( QString() );
        m_nameLabel->clear();
        return;
    }

    m_dateLabel->setText( tr( "Unknown Release-Date" ) );

    connect( m_query->track().data(), SIGNAL( updated() ), SLOT( onCoverUpdated() ) );
    connect( m_query->track().data(), SIGNAL( socialActionsLoaded() ), SLOT( onSocialActionsLoaded() ) );
    connect( m_query.data(), SIGNAL( resultsChanged() ), SLOT( onResultsChanged() ), Qt::QueuedConnection );
    connect( m_query.data(), SIGNAL( resultsChanged() ), SLOT( onAlbumUpdated() ) );
}


void
TrackDetailView::onAlbumUpdated()
{
    if ( !m_query )
        return;

    if ( m_query->track()->albumPtr() && !m_query->track()->albumPtr()->name().isEmpty() )
    {
        m_nameLabel->setType( QueryLabel::Album );
        m_nameLabel->setAlbum( m_query->track()->albumPtr() );

        connect( m_query->track()->albumPtr().data(), SIGNAL( updated() ), SLOT( onAlbumUpdated() ), Qt::UniqueConnection );

        if ( m_buyButtonVisible )
        {
            if ( m_query->track()->albumPtr()->purchased() )
            {
                m_allTracksAvailableLocally = true;
                foreach( const query_ptr& currentQuery, m_playlistInterface->tracks() )
                {
                    if ( currentQuery->results().isEmpty() )
                    {
                        m_allTracksAvailableLocally = false;
                        break;
                    }
                    else
                    {
                        m_allTracksAvailableLocally = false;
                        foreach ( const result_ptr& currentResult, currentQuery->results() )
                        {
                            QList< DownloadFormat > formats = currentResult->downloadFormats();
                            bool isDownloaded = formats.isEmpty() ? false : !DownloadManager::instance()->localFileForDownload( currentResult->downloadFormats().first().url.toString() ).isEmpty();
                            if ( currentResult->isLocal() || isDownloaded )
                            {
                                m_allTracksAvailableLocally = true;
                                break;
                            }
                        }
                        if ( !m_allTracksAvailableLocally )
                        {
                            break;
                        }
                    }
                }

                if ( m_allTracksAvailableLocally )
                {
                    m_buyButton->setText( tr( "View in Folder" ) );
                    m_buyButton->setVisible( true );
                }
                else
                {
                    m_buyButton->setText( tr( "Download Album" ) );
                    m_buyButton->setVisible( true );
                }
            }
            else
            {
                m_buyButton->setText( tr( "Buy Album" ) );
                m_buyButton->setVisible( !m_query->track()->albumPtr()->purchaseUrl().isEmpty() );
            }
        }
    }
    else
    {
        m_nameLabel->setType( QueryLabel::Artist );
        m_nameLabel->setArtist( m_query->track()->artistPtr() );
        m_buyButton->setVisible( false );
    }
}


void
TrackDetailView::onBuyButtonClicked()
{
    if ( DownloadManager::instance()->state() == DownloadManager::Running )
    {
        emit downloadCancel();
        return;
    }

    if ( m_query && m_query->track()->albumPtr() )
    {
        if ( m_query->track()->albumPtr()->purchased() )
        {
            if ( m_allTracksAvailableLocally )
            {
                QDesktopServices::openUrl( QUrl::fromLocalFile( DownloadJob::localPath( m_query->track()->albumPtr() ) ) );
            }
            else
            {
                emit downloadAll();
            }
        }
        else
        {
            WebPopup* popup = new WebPopup( m_query->track()->albumPtr()->purchaseUrl(), QSize( 400, 800 ) );
            connect( m_query->track()->albumPtr().data(), SIGNAL( destroyed() ), popup, SLOT( close() ) );
        }
    }
}


void
TrackDetailView::onDownloadManagerStateChanged( DownloadManager::DownloadManagerState newState, DownloadManager::DownloadManagerState oldState )
{
    tDebug() << Q_FUNC_INFO;
    if ( newState == DownloadManager::Running )
    {
        m_buyButton->setText( tr( "Cancel Download" ) );
    }
    else
    {
        onAlbumUpdated();
    }
}


void
TrackDetailView::onCoverUpdated()
{
    if ( !m_query || m_query->track()->cover( QSize( 0, 0 ) ).isNull() )
    {
        m_playableCover->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultTrackImage, TomahawkUtils::Original, m_playableCover->size() ) );
        return;
    }

    m_pixmap = m_query->track()->cover( m_playableCover->size() );
    m_playableCover->setPixmap( m_pixmap );
}


void
TrackDetailView::onSocialActionsLoaded()
{
    Track* track = qobject_cast< Track* >( sender() );
    if ( !track || !m_query || !track->equals( m_query->track() ) )
        return;

    setSocialActions();
}


void
TrackDetailView::setSocialActions()
{
    if ( m_query && m_query->track()->loved() )
    {
        m_lovedIcon->setVisible( true );
        m_lovedLabel->setVisible( true );
        m_infoBox->show();
    }
    else
    {
        m_lovedIcon->setVisible( false );
        m_lovedLabel->setVisible( false );
        m_infoBox->hide();
    }
}


void
TrackDetailView::onResultsChanged()
{
    QLayoutItem* child;
    while ( ( child = m_resultsBox->layout()->takeAt( 0 ) ) != 0 )
    {
        delete child->widget();
        delete child;
    }

    if ( m_query )
    {
        QFont f = font();
        f.setPointSize( TomahawkUtils::defaultFontSize() );

        foreach ( const Tomahawk::result_ptr& result, m_query->results() )
        {
            if ( !result->isOnline() )
                continue;

            QLabel* resolverIcon = new QLabel( this );
            resolverIcon->setFixedWidth( 12 );
            resolverIcon->setPixmap( result->sourceIcon( TomahawkUtils::RoundedCorners, QSize( 12, 12 ) ) );

            ClickableLabel* resolverLabel = new ClickableLabel( this );
            resolverLabel->setFont( f );
            resolverLabel->setStyleSheet( "QLabel { color: rgba( 0, 0, 0, 50% ) }" );
            resolverLabel->setText( QString( "%1 - %2" ).arg( result->track()->track() ).arg( result->track()->artist() ) );
            resolverLabel->setToolTip(
                QString( "%1 by %2%3 (%4)" )
                .arg( result->track()->track() )
                .arg( result->track()->artist() )
                .arg( !result->track()->album().isEmpty() ? QString( " " ) + tr( "on %1" ).arg( result->track()->album() ) : QString() )
                .arg( result->friendlySource() )
            );

                                                            ;
            resolverLabel->setFixedWidth( width() - 32 - 4 );

            NewClosure( resolverLabel, SIGNAL( clicked() ), const_cast< TrackDetailView* >( this ),
                                        SLOT( onResultClicked( Tomahawk::playlistinterface_ptr, Tomahawk::result_ptr, Tomahawk::query_ptr ) ),
                                        m_playlistInterface, result, m_query )->setAutoDelete( false );

            QWidget* hbox = new QWidget;
            QHBoxLayout* hboxl = new QHBoxLayout;
            TomahawkUtils::unmarginLayout( hboxl );
            hboxl->setSpacing( 8 );
            hboxl->addWidget( resolverIcon );
            hboxl->addWidget( resolverLabel );
            hbox->setLayout( hboxl );

            m_resultsBox->layout()->addWidget( hbox );
        }

        qobject_cast<QVBoxLayout*>(m_resultsBox->layout())->addStretch();
    }

    if ( m_query && m_query->numResults( true ) > 1 )
    {
        m_resultsBoxLabel->show();
        m_resultsBox->show();
        m_resultsScrollArea->show();
    }
    else
    {
        m_resultsBoxLabel->hide();
        m_resultsBox->hide();
        m_resultsScrollArea->hide();
    }
}


void
TrackDetailView::setBuyButtonVisible( bool visible )
{
    m_buyButtonVisible = visible;
}


void
TrackDetailView::onResultClicked( const Tomahawk::playlistinterface_ptr& playlist, const Tomahawk::result_ptr& result, const Tomahawk::query_ptr& fromQuery )
{
    fromQuery->setPreferredResult( result );
    if (AudioEngine::instance()->isPlaying() && fromQuery->results().contains( AudioEngine::instance()->currentTrack() )) {
        AudioEngine::instance()->playItem( playlist, result, fromQuery );
    }
}
