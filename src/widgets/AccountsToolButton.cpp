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
#include "utils/TomahawkUtils.h"

#include <QLabel>
#include <QListView>
#include <QMouseEvent>
#include <QVBoxLayout>

AccountsToolButton::AccountsToolButton( QWidget* parent )
    : QToolButton( parent )
{
    m_popup = new AccountsPopupWidget( this );
    m_popup->hide();

    //Set up popup...
    QWidget *w = new QWidget( this );
    w->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );
    QVBoxLayout *wMainLayout = new QVBoxLayout( w );
    w->setLayout( wMainLayout  );
    QLabel *connectionsLabel = new QLabel( tr( "Connections" ), w );

    QFont clFont = connectionsLabel->font();
    clFont.setBold( true );
    clFont.setPointSize( 12 );
    connectionsLabel->setFont( clFont );
    connectionsLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );

    QToolButton *settingsButton = new QToolButton( w );
    settingsButton->setIcon( QIcon( RESPATH "images/account-settings.png" ) );
    settingsButton->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
    settingsButton->setText( tr( "Configure Accounts" ) );
    connect( settingsButton, SIGNAL( clicked() ),
             window(), SLOT( showSettingsDialog() ) );

    QHBoxLayout *headerLayout = new QHBoxLayout( w );
    headerLayout->addWidget( connectionsLabel );
    headerLayout->addSpacing( 30 );
    headerLayout->addWidget( settingsButton );
    wMainLayout->addLayout( headerLayout );
    QWidget *separatorLine = new QWidget( w );
    separatorLine->setFixedHeight( 1 );
    separatorLine->setContentsMargins( 0, 0, 0, 0 );
    separatorLine->setStyleSheet( "QWidget { border-top: 1px solid black; }" );
    wMainLayout->addWidget( separatorLine );

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
}

void
AccountsToolButton::mousePressEvent( QMouseEvent* event )
{
    if( m_popup )
    {
        QPoint myPos = mapToGlobal( rect().bottomRight() );
        m_popup->anchorAt( myPos );
        m_popup->show();
        event->accept();
    }
    QToolButton::mousePressEvent( event );
}

void
AccountsToolButton::popupHidden() //SLOT
{
    setDown( false );
}
