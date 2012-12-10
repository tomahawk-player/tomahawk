/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012,      Teo Mrnjavac <teo@kde.org>
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

#include "AccountsToolButton.h"

#include "AccountListWidget.h"
#include "accounts/AccountManager.h"
#include "utils/TomahawkUtilsGui.h"

#include <QLabel>
#include <QListView>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QToolBar>
#include <QVBoxLayout>

AccountsToolButton::AccountsToolButton( QWidget* parent )
    : QToolButton( parent )
{
    m_popup = new AccountsPopupWidget( this );
    m_popup->hide();

    QToolBar* toolbar = qobject_cast< QToolBar* >( parent );
    if ( toolbar )
        setIconSize( toolbar->iconSize() );
    else
        setIconSize( QSize( 22, 22 ) );

    //Set up popup...
    QWidget *w = new QWidget( this );
    w->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );
    QVBoxLayout *wMainLayout = new QVBoxLayout( w );
    w->setLayout( wMainLayout  );

    TomahawkUtils::unmarginLayout( w->layout() );

    w->setContentsMargins( 6, 6, 6, 6 );
#ifdef Q_OS_MAC
    w->setContentsMargins( 6, 6, 6, 0 );
    wMainLayout->setMargin( 12 );
#endif

    m_popup->setWidget( w );
    connect( m_popup, SIGNAL( hidden() ), SLOT( popupHidden() ) );

    m_model = new Tomahawk::Accounts::AccountModel( this );
    m_proxy = new AccountModelFactoryProxy( m_model );
    m_proxy->setSourceModel( m_model );
    m_proxy->setFilterRowType( Tomahawk::Accounts::AccountModel::TopLevelFactory );
    m_proxy->setFilterEnabled( true );

    AccountListWidget *view = new AccountListWidget( m_proxy, m_popup );
    wMainLayout->addWidget( view );
    view->setAutoFillBackground( false );
    view->setAttribute( Qt::WA_TranslucentBackground, true );

    connect( m_proxy, SIGNAL( dataChanged( QModelIndex, QModelIndex ) ),
             this, SLOT( repaint() ) );

    QWidget *separatorLine = new QWidget( w );
    separatorLine->setFixedHeight( 1 );
    separatorLine->setContentsMargins( 0, 0, 0, 0 );
    separatorLine->setStyleSheet( "QWidget { border-top: 1px solid " +
                                  TomahawkUtils::Colors::BORDER_LINE.name() + "; }" ); //from ProxyStyle
    wMainLayout->addWidget( separatorLine );

    wMainLayout->addSpacing( 6 );

    QPushButton *settingsButton = new QPushButton( w );
    settingsButton->setIcon( TomahawkUtils::defaultPixmap( TomahawkUtils::AccountSettings ) );
    settingsButton->setText( tr( "Configure Accounts" ) );
    connect( settingsButton, SIGNAL( clicked() ), window(), SLOT( showSettingsDialog() ) );

    QHBoxLayout *bottomLayout = new QHBoxLayout( w );
    TomahawkUtils::unmarginLayout( bottomLayout );
    bottomLayout->addStretch();
    bottomLayout->addWidget( settingsButton );
    wMainLayout->addLayout( bottomLayout );

    connect( m_proxy, SIGNAL( dataChanged( QModelIndex, QModelIndex ) ),
             this, SLOT( updateIcons() ) );
    connect( m_proxy, SIGNAL( rowsInserted ( QModelIndex, int, int ) ),
             this, SLOT( updateIcons() ) );
    connect( m_proxy, SIGNAL( rowsRemoved( QModelIndex, int, int ) ),
             this, SLOT( updateIcons() ) );
    connect( m_proxy, SIGNAL( modelReset() ),
             this, SLOT( updateIcons() ) );

#ifdef Q_WS_MAC
    setIconSize( QSize( iconSize().width()-4, iconSize().height()-4 ) );
    setStyleSheet( "AccountsToolButton {"
                   "    border: 1px solid #7d7d7d;"
                   "    border-radius: 4px;"
                   "    background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
                   "                                      stop: 0 #f5f5f5, stop: 1 #bababa);"
                   "}"
                   "AccountsToolButton:pressed {"
                   "    background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
                   "                                      stop: 0 #b6b6b6, stop: 1 #dddddd);"
                   "}" );
#endif
}

void
AccountsToolButton::mousePressEvent( QMouseEvent* event )
{
    QToolButton::mousePressEvent( event );
    if( m_popup )
    {
        QPoint myPos = mapToGlobal( rect().bottomRight() );
        m_popup->anchorAt( myPos );
        m_popup->setArrowOffset( rect().width() / 2 );
        if ( !m_popup->isVisible() )
            m_popup->show();
        else
            m_popup->hide();
        event->accept();
    }
}

void
AccountsToolButton::paintEvent( QPaintEvent* event )
{
    QToolButton::paintEvent( event );

    QPainter painter( this );
    painter.initFrom( this );

    if ( m_proxy->rowCount() == 0 )
    {
        QRect pixmapRect( QPoint( width() / 2 - iconSize().width() / 2,
                                  height() / 2 - iconSize().height() / 2 ),
                          iconSize() );

        painter.drawPixmap( pixmapRect, TomahawkUtils::defaultPixmap( TomahawkUtils::AccountNone, TomahawkUtils::Original, iconSize() ) );
    }
    else
    {
        for ( int i = 0; i < m_factoryPixmaps.count(); ++i )
        {
            int diff = height() - iconSize().height();
            int pixmapRectX = diff / 2
                            + i * ( iconSize().width() + diff );
            QRect pixmapRect( QPoint( pixmapRectX, height() / 2 - iconSize().height() / 2 ),
                              iconSize() );
            painter.drawPixmap( pixmapRect, m_factoryPixmaps.at( i ) );
        }
    }

    painter.end();
}

void
AccountsToolButton::moveEvent( QMoveEvent* event )
{
    Q_UNUSED( event );

    if ( m_popup )
    {
        if ( isDown() )
        {
            QPoint myPos = mapToGlobal( rect().bottomRight() );
            m_popup->anchorAt( myPos );
        }
    }
}

void
AccountsToolButton::popupHidden() //SLOT
{
    setDown( false );
}

void
AccountsToolButton::updateIcons()
{
    m_factoryPixmaps.clear();
    int oldWidth = sizeHint().width();

    for ( int i = 0; i < m_proxy->rowCount(); ++i )
    {
        QModelIndex idx = m_proxy->index( i, 0 );
        const QList< Tomahawk::Accounts::Account* >& children =
                idx.data( Tomahawk::Accounts::AccountModel::ChildrenOfFactoryRole )
                   .value< QList< Tomahawk::Accounts::Account* > >();
        int count = children.count();

        if ( count == 0 )
            continue;

        if ( count == 1 )
        {
            m_factoryPixmaps.append( children.first()->icon()
                                                .scaled( iconSize(),
                                                         Qt::KeepAspectRatio,
                                                         Qt::SmoothTransformation ) );
        }
        else //we need to find if at least one of this factory's accounts is connected
        {
            int connectedAccountIndex = -1;
            for ( int j = 0; j < count; ++j )
            {
                if ( children.at( j )->connectionState() ==
                     Tomahawk::Accounts::Account::Connected )
                {
                    connectedAccountIndex = j;
                    break;
                }
            }
            if ( connectedAccountIndex != -1 )
            {
                m_factoryPixmaps.append( children.at( connectedAccountIndex )->icon()
                                                        .scaled( iconSize(),
                                                                 Qt::KeepAspectRatio,
                                                                 Qt::SmoothTransformation ) );
            }
            else
            {
                m_factoryPixmaps.append( children.first()->icon()
                                                    .scaled( iconSize(),
                                                             Qt::KeepAspectRatio,
                                                             Qt::SmoothTransformation ) );
            }
        }
    }

    resize( sizeHint() );
    if ( oldWidth != sizeHint().width() )
        emit widthChanged();

    m_popup->setArrowOffset( rect().width() / 2 );

    repaint();
}


QSize
AccountsToolButton::sizeHint() const
{
    QSize size = QToolButton::sizeHint();
    if ( m_factoryPixmaps.count() == 0 ) //no accounts enabled!
        return size;

    size.rwidth() *= m_factoryPixmaps.count();
    return size;
}
