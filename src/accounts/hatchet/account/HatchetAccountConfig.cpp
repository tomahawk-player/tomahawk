/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Leo Franchi <lfranchi@kde.org>
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

#include "HatchetAccountConfig.h"
#include "HatchetAccount.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

#include "ui_HatchetAccountConfig.h"

#include <QMessageBox>

using namespace Tomahawk;
using namespace Accounts;

namespace {
    enum ButtonAction {
        Login,
        Register,
        Logout
    };
}

HatchetAccountConfig::HatchetAccountConfig( HatchetAccount* account )
    : AccountConfigWidget( 0 )
    , m_ui( new Ui::HatchetAccountConfig )
    , m_account( account )
{
    Q_ASSERT( m_account );

    m_ui->setupUi( this );

    m_ui->label->setPixmap( m_ui->label->pixmap()->scaled( QSize( 128, 127 ), Qt::KeepAspectRatio, Qt::SmoothTransformation ) );

    m_ui->loginButton->setDefault( true );
    connect( m_ui->loginButton, SIGNAL( clicked( bool ) ), this, SLOT( login() ) );

    connect( m_ui->usernameEdit, SIGNAL( textChanged( QString ) ), this, SLOT( fieldsChanged() ) );
    connect( m_ui->passwordEdit, SIGNAL( textChanged( QString ) ), this, SLOT( fieldsChanged() ) );
    connect( m_ui->otpEdit, SIGNAL( textChanged( QString ) ), this, SLOT( fieldsChanged() ) );

    connect( m_account, SIGNAL( authError( QString, int, QVariantMap ) ), this, SLOT( authError( QString, int, QVariantMap ) ) );
    connect( m_account, SIGNAL( deauthenticated() ), this, SLOT( showLoggedOut() ) );
    connect( m_account, SIGNAL( accessTokenFetched() ), this, SLOT( accountInfoUpdated() ) );

    if ( !m_account->refreshToken().isEmpty() )
        accountInfoUpdated();
    else
    {
        m_ui->usernameEdit->setText( m_account->username() );
        showLoggedOut();
    }
}

HatchetAccountConfig::~HatchetAccountConfig()
{

}


void
HatchetAccountConfig::login()
{
    tLog() << Q_FUNC_INFO;
    const ButtonAction action = static_cast< ButtonAction>( m_ui->loginButton->property( "action" ).toInt() );

    if ( action == Login )
    {
        // Log in mode
        tLog() << Q_FUNC_INFO << "Logging in...";
        m_account->loginWithPassword( m_ui->usernameEdit->text(), m_ui->passwordEdit->text(), m_ui->otpEdit->text() );
    }
    else if ( action == Logout )
    {
        // TODO
        m_ui->usernameEdit->clear();
        m_ui->passwordEdit->clear();
        m_ui->otpEdit->clear();

        QVariantHash creds = m_account->credentials();
        creds.clear();
        m_account->setCredentials( creds );
        m_account->sync();
        m_account->deauthenticate();
    }
}


void
HatchetAccountConfig::fieldsChanged()
{
    const QString username = m_ui->usernameEdit->text();
    const QString password = m_ui->passwordEdit->text();

    const ButtonAction action = static_cast< ButtonAction>( m_ui->loginButton->property( "action" ).toInt() );

    m_ui->loginButton->setEnabled( !username.isEmpty() && !password.isEmpty() && action == Login );

    m_ui->errorLabel->clear();
}


void
HatchetAccountConfig::showLoggedIn()
{
    m_ui->usernameLabel->hide();
    m_ui->usernameEdit->hide();
    m_ui->otpLabel->hide();
    m_ui->otpEdit->hide();
    m_ui->passwordLabel->hide();
    m_ui->passwordEdit->hide();

    m_ui->loggedInLabel->setText( tr( "Logged in as: %1" ).arg( m_account->username() ) );
    m_ui->loggedInLabel->show();

    m_ui->errorLabel->clear();
    m_ui->errorLabel->hide();

    m_ui->loginButton->setText( tr("Log out") );
    m_ui->loginButton->setProperty( "action", Logout );
    m_ui->loginButton->setDefault( true );
}


void
HatchetAccountConfig::showLoggedOut()
{
    m_ui->usernameLabel->show();
    m_ui->usernameEdit->show();
    m_ui->passwordLabel->show();
    m_ui->passwordEdit->show();
    m_ui->otpLabel->hide();
    m_ui->otpEdit->hide();
    m_ui->otpEdit->clear();

    m_ui->loggedInLabel->clear();
    m_ui->loggedInLabel->hide();

    m_ui->errorLabel->clear();

    m_ui->loginButton->setText( tr("Log in") );
    m_ui->loginButton->setProperty( "action", Login );
    m_ui->loginButton->setDefault( true );
}


void
HatchetAccountConfig::accountInfoUpdated()
{
    showLoggedIn();
    return;
}


void
HatchetAccountConfig::authError( const QString &error, int statusCode, const QVariantMap& resp )
{
    if ( statusCode == 400 && error == "otp_needed" )
    {
        m_ui->usernameLabel->hide();
        m_ui->usernameEdit->hide();
        m_ui->otpLabel->show();
        m_ui->otpEdit->show();
        m_ui->passwordLabel->hide();
        m_ui->passwordEdit->hide();
        m_ui->loginButton->setText( tr("Continue") );
        return;
    }
    if ( statusCode == 401 )
        m_account->deauthenticate();
    QMessageBox::critical( this, "An error was encountered:", error );
}


void
HatchetAccountConfig::showEvent( QShowEvent *event )
{
    AccountConfigWidget::showEvent( event );
    m_ui->loginButton->setDefault( true );
}
