/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012-2013, Teo Mrnjavac <teo@kde.org>
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

#include "AccountWidget.h"

#include "UnstyledFrame.h"
#include "SlideSwitchButton.h"
#include "accounts/Account.h"
#include "accounts/AccountModel.h"
#include "accounts/AccountManager.h"
#include "sip/SipPlugin.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/AnimatedSpinner.h"
#include "widgets/ElidedLabel.h"

#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "sip/SipStatusMessage.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>

AccountWidget::AccountWidget( QWidget* parent )
    : QWidget( parent )
{
    QHBoxLayout *mainLayout = new QHBoxLayout( this );
    TomahawkUtils::unmarginLayout( mainLayout );
    setLayout( mainLayout );
    setContentsMargins( 0, 8, 0, 8 );

    m_imageLabel = new QLabel( this );
    mainLayout->addWidget( m_imageLabel );
    mainLayout->setSpacing( 4 );

    QGridLayout* vLayout = new QGridLayout( this );
    vLayout->setSpacing( 8 );
    mainLayout->addLayout( vLayout );

    QFrame* idContainer = new QFrame( this );
    idContainer->setAttribute( Qt::WA_TranslucentBackground, false );
    vLayout->addWidget( idContainer, 0, 0 );

    QHBoxLayout* idContLayout = new QHBoxLayout( idContainer );
    idContainer->setLayout( idContLayout );
    idContainer->setContentsMargins( 0, 0, 0, 0 );
    idContLayout->setMargin( 2 );

    m_idLabel = new ElidedLabel( idContainer );
    m_idLabel->setElideMode( Qt::ElideRight );
    m_idLabel->setContentsMargins( 3, 0, 3, 0 );
    m_idLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_idLabel->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    idContLayout->addWidget( m_idLabel );

    m_spinnerWidget = new QWidget( idContainer );
    QSize spinnerSize = 16 > TomahawkUtils::defaultFontHeight()  ?
                            QSize( 16, 16 ) :
                            QSize( TomahawkUtils::defaultFontHeight(),
                                   TomahawkUtils::defaultFontHeight() );
    m_spinnerWidget->setFixedSize( spinnerSize );
    idContLayout->addWidget( m_spinnerWidget );
    m_spinnerWidget->setContentsMargins( 0, 1, 0, 0 );
    m_spinner = new AnimatedSpinner( m_spinnerWidget->size() - QSize( 2, 2 ), m_spinnerWidget );

    idContainer->setStyleSheet( QString( "QFrame {"
                                "border: 1px solid #e9e9e9;"
                                "border-radius: %1px;"
                                "background: #e9e9e9;"
                                "}" ).arg( idContainer->sizeHint().height() / 2 + 1 ) );
    idContainer->setMinimumHeight( spinnerSize.height() + 6 /*margins*/ );

    m_statusToggle = new SlideSwitchButton( this );
    m_statusToggle->setContentsMargins( 0, 0, 0, 0 );
    m_statusToggle->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Expanding );
    m_statusToggle->setFixedSize( m_statusToggle->sizeHint() );
    QHBoxLayout *statusToggleLayout = new QHBoxLayout( this );
    vLayout->addLayout( statusToggleLayout, 0, 1, 1, 1 );
    statusToggleLayout->addStretch();
    statusToggleLayout->addWidget( m_statusToggle );

    m_inviteContainer = new UnstyledFrame( this );
    vLayout->addWidget( m_inviteContainer, 1, 0 );
    m_inviteContainer->setFrameColor( TomahawkUtils::Colors::BORDER_LINE );
    m_inviteContainer->setMinimumWidth( m_inviteContainer->logicalDpiX() * 2 );
    m_inviteContainer->setContentsMargins( 1, 1, 1, 2 );
    m_inviteContainer->setAttribute( Qt::WA_TranslucentBackground, false );
    m_inviteContainer->setStyleSheet( "background: white" );

    QHBoxLayout* containerLayout = new QHBoxLayout( m_inviteContainer );
    m_inviteContainer->setLayout( containerLayout );
    TomahawkUtils::unmarginLayout( containerLayout );
    containerLayout->setContentsMargins( 1, 1, 0, 0 );

    m_addAccountIcon = new QLabel( m_inviteContainer );
    m_addAccountIcon->setContentsMargins( 1, 0, 0, 0 );
    m_addAccountIcon->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::AddContact, TomahawkUtils::Original, QSize( 16, 16 ) ) );
    m_addAccountIcon->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Expanding );
    m_addAccountIcon->setAlignment( Qt::AlignCenter );
    containerLayout->addWidget( m_addAccountIcon );

    m_inviteEdit = new QLineEdit( m_inviteContainer );
    m_inviteEdit->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    containerLayout->addWidget( m_inviteEdit );
    m_inviteEdit->setFrame( false );

    m_inviteButton = new QPushButton( this );
    m_inviteButton->setMinimumWidth( m_inviteButton->logicalDpiX() * 0.8 );
    m_inviteButton->setText( tr( "Invite" ) );
    m_inviteButton->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
    vLayout->addWidget( m_inviteButton, 1, 1 );
    vLayout->setColumnStretch( 0, 1 );


#ifdef Q_OS_MAC
    layout()->setContentsMargins( 0, 0, 0, 0 );
#endif

    setInviteWidgetsEnabled( false );
}

AccountWidget::~AccountWidget()
{
    delete m_spinner;
}


void
AccountWidget::update( const QPersistentModelIndex& idx, int accountIdx )
{
    Tomahawk::Accounts::Account* account =
            idx.data( Tomahawk::Accounts::AccountModel::ChildrenOfFactoryRole )
            .value< QList< Tomahawk::Accounts::Account* > >().at( accountIdx );
    if ( account )
    {
        const QPixmap& pixmap = account->icon();
        QSize pixmapSize( 32, 32 );
        m_imageLabel->setPixmap( pixmap.scaled( pixmapSize, Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
        m_imageLabel->setFixedSize( pixmapSize );

        QFontMetrics fm = m_idLabel->fontMetrics();
        m_idLabel->setText( account->accountFriendlyName() );
        m_idLabel->setToolTip( "<b>" +
                               account->accountServiceName() +
                               "</b><br>" +
                               account->accountFriendlyName() );

        //we already know it's a factory because of the FactoryProxy
        Tomahawk::Accounts::AccountFactory* fac =
                qobject_cast< Tomahawk::Accounts::AccountFactory* >(
                    idx.data( Tomahawk::Accounts::AccountModel::AccountData )
                        .value< QObject* >() );
        if ( fac->factoryId() == "twitteraccount" )
        {
            m_inviteContainer->setVisible( false );
            m_inviteButton->setVisible( false );
        }

        switch ( account->connectionState() )
        {
        case Tomahawk::Accounts::Account::Connected:
            if ( account->enabled() )
                m_statusToggle->setChecked( true );
            else
                qDebug() << "AccountWidget warning:" << account->accountFriendlyName()
                         << "is Connected but Disabled!";
            m_spinner->fadeOut();
            m_statusToggle->setBackChecked( true );
            setInviteWidgetsEnabled( true );
            break;
        case Tomahawk::Accounts::Account::Connecting:
            if ( !account->enabled() )
                qDebug() << "AccountWidget warning:" << account->accountFriendlyName()
                         << "is Connecting but Disabled!";
            m_spinner->fadeIn();
            m_statusToggle->setBackChecked( false );
            setInviteWidgetsEnabled( false );
            break;
        case Tomahawk::Accounts::Account::Disconnected:
            if ( !account->enabled() )
                m_statusToggle->setChecked( false );
            else
                qDebug() << "AccountWidget warning:" << account->accountFriendlyName()
                         << "is Disconnected but Enabled!";
            m_spinner->fadeOut();
            m_statusToggle->setBackChecked( false );
            setInviteWidgetsEnabled( false );
            break;
        case Tomahawk::Accounts::Account::Disconnecting:
            if ( account->enabled() )
                qDebug() << "AccountWidget warning:" << account->accountFriendlyName()
                         << "is Disconnecting but Enabled!";
            m_spinner->fadeIn();
            m_statusToggle->setBackChecked( true );
            setInviteWidgetsEnabled( false );
        }

        if ( !account->enabled() && account->connectionState() == Tomahawk::Accounts::Account::Disconnected )
        {
            m_spinner->fadeOut();
            m_statusToggle->setBackChecked( false );
            m_statusToggle->setChecked( false );
            setInviteWidgetsEnabled( false );
        }
        else if ( account->enabled() && account->connectionState() == Tomahawk::Accounts::Account::Connected )
        {
            m_spinner->fadeOut();
            m_statusToggle->setBackChecked( true );
            m_statusToggle->setChecked( true );
            setInviteWidgetsEnabled( true );
        }
    }
}

void
AccountWidget::changeAccountConnectionState( bool connected )
{
    Tomahawk::Accounts::Account* account =
            m_myFactoryIdx.data( Tomahawk::Accounts::AccountModel::ChildrenOfFactoryRole )
            .value< QList< Tomahawk::Accounts::Account* > >().at( m_myAccountIdx );
    if ( account )
    {
        if ( connected )
        {
            Tomahawk::Accounts::AccountManager::instance()->enableAccount( account );
        }
        else
        {
            Tomahawk::Accounts::AccountManager::instance()->disableAccount( account );
        }
    }
}

void
AccountWidget::sendInvite()
{
    Tomahawk::Accounts::Account* account =
            m_myFactoryIdx.data( Tomahawk::Accounts::AccountModel::ChildrenOfFactoryRole )
            .value< QList< Tomahawk::Accounts::Account* > >().at( m_myAccountIdx );
    if ( account )
    {
        if ( !m_inviteEdit->text().isEmpty() )
            account->sipPlugin()->addContact( m_inviteEdit->text() );
        m_inviteButton->setEnabled( false );
        m_inviteEdit->setEnabled( false );
        QTimer::singleShot( 500, this, SLOT( clearInviteWidgets() ) );
    }
}


void
AccountWidget::onInviteSentSuccess( const QString& inviteId )
{
    JobStatusView::instance()->model()->addJob( new SipStatusMessage( SipStatusMessage::SipInviteSuccess, inviteId ) );
}

void
AccountWidget::onInviteSentFailure( const QString& inviteId )
{
    JobStatusView::instance()->model()->addJob( new SipStatusMessage( SipStatusMessage::SipInviteFailure, inviteId ) );
}


void
AccountWidget::clearInviteWidgets()
{
    setInviteWidgetsEnabled( m_statusToggle->backChecked() );
    m_inviteEdit->clear();
}

void
AccountWidget::setInviteWidgetsEnabled( bool enabled )
{
    m_inviteButton->setEnabled( enabled );
    m_inviteEdit->setEnabled( enabled );
}

void
AccountWidget::setupConnections( const QPersistentModelIndex& idx, int accountIdx )
{
    m_myFactoryIdx = idx;
    m_myAccountIdx = accountIdx;

    Tomahawk::Accounts::Account* account =
            idx.data( Tomahawk::Accounts::AccountModel::ChildrenOfFactoryRole )
            .value< QList< Tomahawk::Accounts::Account* > >().at( accountIdx );
    if ( account )
    {
        connect( m_statusToggle, SIGNAL( toggled( bool ) ),
                 this, SLOT( changeAccountConnectionState( bool ) ) );
        connect( m_inviteButton, SIGNAL( clicked() ),
                 this, SLOT( sendInvite() ) );
        connect( m_inviteEdit, SIGNAL( returnPressed() ),
                 this, SLOT( sendInvite() ) );

        if ( account->sipPlugin() )
        {
            m_inviteEdit->setPlaceholderText( account->sipPlugin()->inviteString() );
            connect( account->sipPlugin(), SIGNAL( inviteSentSuccess( QString ) ), SLOT( onInviteSentSuccess( QString ) ) );
            connect( account->sipPlugin(), SIGNAL( inviteSentFailure( QString ) ), SLOT( onInviteSentFailure( QString ) ) );
        }
    }
}

void
AccountWidget::setConnectionState( bool state )
{
    m_statusToggle->setChecked( state );
}

bool AccountWidget::connectionState() const
{
    return m_statusToggle->isChecked() || m_statusToggle->backChecked();
}
