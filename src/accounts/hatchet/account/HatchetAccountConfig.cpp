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

    m_ui->emailLabel->hide();
    m_ui->emailEdit->hide();

    connect( m_ui->registerbutton, SIGNAL( clicked( bool ) ), this, SLOT( registerClicked() ) );
    connect( m_ui->loginOrRegisterButton, SIGNAL( clicked( bool ) ), this, SLOT( loginOrRegister() ) );

    connect( m_ui->usernameEdit, SIGNAL( textChanged( QString ) ), this, SLOT( fieldsChanged() ) );
    connect( m_ui->passwordEdit, SIGNAL( textChanged( QString ) ), this, SLOT( fieldsChanged() ) );
    connect( m_ui->emailEdit, SIGNAL( textChanged( QString ) ), this, SLOT( fieldsChanged() ) );

    connect( m_account, SIGNAL( registerFinished( bool, QString ) ), this, SLOT( registerFinished( bool, QString ) ) );
    connect( m_account, SIGNAL( deauthenticated() ), this, SLOT( showLoggedOut() ) );
    connect( m_account, SIGNAL( accessTokensFetched() ), this, SLOT( accountInfoUpdated() ) );

    if ( !m_account->authToken().isEmpty() )
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
HatchetAccountConfig::registerClicked()
{
    m_ui->registerbutton->hide();

    m_ui->emailLabel->show();
    m_ui->emailEdit->show();
    m_ui->loginOrRegisterButton->setText( tr( "Register" ) );
    m_ui->loginOrRegisterButton->setProperty( "action", Register );

}


void
HatchetAccountConfig::loginOrRegister()
{
    const ButtonAction action = static_cast< ButtonAction>( m_ui->loginOrRegisterButton->property( "action" ).toInt() );

    if ( action == Login )
    {
        // Log in mode
        m_account->loginWithPassword( m_ui->usernameEdit->text(), m_ui->passwordEdit->text() );
    }
    else if ( action == Register )
    {
        // Register since the use clicked register and just entered his info
        const QString username = m_ui->usernameEdit->text();
        const QString password = m_ui->passwordEdit->text();
        const QString email = m_ui->emailEdit->text();
        m_account->doRegister( username, password, email );
    }
    else if ( action == Logout )
    {
        // TODO
        m_ui->usernameEdit->clear();
        m_ui->passwordEdit->clear();

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
    const QString email = m_ui->emailEdit->text();

    const ButtonAction action = static_cast< ButtonAction>( m_ui->loginOrRegisterButton->property( "action" ).toInt() );

    m_ui->loginOrRegisterButton->setEnabled( !username.isEmpty() && !password.isEmpty() && ( action == Login || !email.isEmpty() ) );

    m_ui->errorLabel->clear();

    if ( action == Login )
        m_ui->loginOrRegisterButton->setText( tr( "Login" ) );
    else if ( action == Register )
        m_ui->loginOrRegisterButton->setText( tr( "Register" ) );
}


void
HatchetAccountConfig::registerFinished( bool success, const QString& error )
{
    if ( success )
    {
        showLoggedOut();
        m_ui->errorLabel->setText( tr( "An email has been sent to activate your account" ) );
    }
    else
    {
        m_ui->loginOrRegisterButton->setText( "Failed" );
        m_ui->loginOrRegisterButton->setEnabled( false );
        m_ui->errorLabel->setText( error );
    }
}


void
HatchetAccountConfig::showLoggedIn()
{
    m_ui->registerbutton->hide();
    m_ui->usernameLabel->hide();
    m_ui->usernameEdit->hide();
    m_ui->emailLabel->hide();
    m_ui->emailEdit->hide();
    m_ui->passwordLabel->hide();
    m_ui->passwordEdit->hide();

    m_ui->loggedInLabel->setText( tr( "Logged in as: %1" ).arg( m_account->username() ) );
    m_ui->loggedInLabel->show();

    m_ui->errorLabel->clear();
    m_ui->errorLabel->hide();

    m_ui->loginOrRegisterButton->setText( "Log out" );
    m_ui->loginOrRegisterButton->setProperty( "action", Logout );
}


void
HatchetAccountConfig::showLoggedOut()
{
    m_ui->emailEdit->hide();
    m_ui->emailLabel->hide();

    m_ui->registerbutton->show();
    m_ui->usernameLabel->show();
    m_ui->usernameEdit->show();
    m_ui->passwordLabel->show();
    m_ui->passwordEdit->show();

    m_ui->loggedInLabel->clear();
    m_ui->loggedInLabel->hide();

    m_ui->errorLabel->clear();

    m_ui->loginOrRegisterButton->setText( "Login" );
    m_ui->loginOrRegisterButton->setProperty( "action", Login );
}


void
HatchetAccountConfig::accountInfoUpdated()
{
    showLoggedIn();
    return;
}
