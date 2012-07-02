/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Leo Franchi <lfranchi@kde.org>
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

#include "SpotifyAccountConfig.h"

#include "SpotifyAccount.h"
#include "utils/AnimatedSpinner.h"
#include "ui_SpotifyAccountConfig.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QShowEvent>
#include <QLabel>

using namespace Tomahawk;
using namespace Accounts;

SpotifyAccountConfig::SpotifyAccountConfig( SpotifyAccount *account )
    : QWidget( 0 )
    , m_ui( new Ui::SpotifyConfig )
    , m_loggedInUser( 0 )
    , m_account( account )
    , m_playlistsLoading( 0 )
    , m_loggedInManually( false )
    , m_isLoggedIn( false )
{
    m_ui->setupUi( this );

    connect( m_ui->loginButton, SIGNAL( clicked( bool ) ), this, SLOT( doLogin() ) );

    connect( m_ui->usernameEdit, SIGNAL( textChanged( QString ) ), this, SLOT( resetLoginButton() ) );
    connect( m_ui->passwordEdit, SIGNAL( textChanged( QString ) ), this, SLOT( resetLoginButton() ) );
    loadFromConfig();

    m_playlistsLoading = new AnimatedSpinner( m_ui->playlistList );
}


void
SpotifyAccountConfig::showEvent( QShowEvent *event )
{
    Q_UNUSED( event );

    loadFromConfig();
    m_loggedInManually = false;
}


void
SpotifyAccountConfig::loadFromConfig()
{
    m_ui->usernameEdit->setText( m_account->credentials().value( "username" ).toString() );
    m_ui->passwordEdit->setText( m_account->credentials().value( "password" ).toString() );
    m_ui->streamingCheckbox->setChecked( m_account->credentials().value( "highQuality" ).toBool() );
    m_ui->deleteOnUnsync->setChecked( m_account->deleteOnUnsync() );
}

void
SpotifyAccountConfig::saveSettings()
{
    for( int i = 0; i < m_ui->playlistList->count(); i++ )
    {
        const QListWidgetItem* item = m_ui->playlistList->item( i );

        SpotifyPlaylistInfo* pl = item->data( Qt::UserRole ).value< SpotifyPlaylistInfo* >();
        const bool toSync = ( item->checkState() == Qt::Checked );
        if ( pl->sync != toSync )
        {
            pl->changed = true;
            pl->sync = toSync;
        }
    }
}


QString
SpotifyAccountConfig::username() const
{
    return m_ui->usernameEdit->text().trimmed();
}

QString
SpotifyAccountConfig::password() const
{
    return m_ui->passwordEdit->text().trimmed();
}

bool
SpotifyAccountConfig::highQuality() const
{
    return m_ui->streamingCheckbox->isChecked();
}


bool
SpotifyAccountConfig::deleteOnUnsync() const
{
    return m_ui->deleteOnUnsync->isChecked();
}


void
SpotifyAccountConfig::setPlaylists( const QList<SpotifyPlaylistInfo *>& playlists )
{
    // User always has at least 1 playlist (starred tracks)
    if ( !playlists.isEmpty() )
        m_playlistsLoading->fadeOut();

    m_ui->playlistList->clear();
    foreach ( SpotifyPlaylistInfo* pl, playlists )
    {
        QListWidgetItem* item = new QListWidgetItem( pl->name, m_ui->playlistList );
        item->setData( Qt::UserRole, QVariant::fromValue< SpotifyPlaylistInfo* >( pl ) );
        item->setFlags( Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled );
        item->setCheckState( pl->sync ? Qt::Checked : Qt::Unchecked );
    }
}


void
SpotifyAccountConfig::doLogin()
{
    if ( !m_isLoggedIn )
    {
        m_ui->loginButton->setText( tr( "Logging in..." ) );
        m_ui->loginButton->setEnabled( false );

        m_playlistsLoading->fadeIn();
        m_loggedInManually = true;

        emit login( username(), password() );
    }
    else
    {
        // Log out
        m_isLoggedIn = false;
        m_loggedInManually = false;
        m_verifiedUsername.clear();
        emit logout();
        showLoggedOut();
    }
}


void
SpotifyAccountConfig::loginResponse( bool success, const QString& msg, const QString& username )
{
    if ( success )
    {
        m_verifiedUsername = username;
        m_isLoggedIn = true;
        showLoggedIn();
    }
    else
    {
        setPlaylists( QList< SpotifyPlaylistInfo* >() );
        m_playlistsLoading->fadeOut();

        m_ui->loginButton->setText( tr( "Failed: %1" ).arg( msg ) );
        m_ui->loginButton->setEnabled( true );
    }

}


void
SpotifyAccountConfig::showLoggedIn()
{
    m_ui->passwordEdit->hide();
    m_ui->passwordLabel->hide();
    m_ui->usernameEdit->hide();
    m_ui->usernameLabel->hide();

    if ( !m_loggedInUser )
    {
        m_loggedInUser = new QLabel( this );
        m_ui->verticalLayout->insertWidget( 1, m_loggedInUser, 0, Qt::AlignCenter );
    }

    m_loggedInUser->setText( tr( "Logged in as %1" ).arg( m_verifiedUsername ) );

    m_ui->loginButton->setText( tr( "Log Out" ) );
    m_ui->loginButton->setEnabled( true );
}


void
SpotifyAccountConfig::showLoggedOut()
{
    m_ui->passwordEdit->show();
    m_ui->passwordLabel->show();
    m_ui->usernameEdit->show();
    m_ui->usernameLabel->show();

    m_loggedInUser->hide();

    m_ui->loginButton->setText( tr( "Log In" ) );
    m_ui->loginButton->setEnabled( true );
}


void
SpotifyAccountConfig::resetLoginButton()
{
    m_ui->loginButton->setText( tr( "Log In" ) );
    m_ui->loginButton->setEnabled( true );
}

