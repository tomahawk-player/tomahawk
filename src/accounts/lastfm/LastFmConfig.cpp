/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "LastFmConfig.h"

#include "LastFmAccount.h"
#include <utils/TomahawkUtils.h>
#include "ui_LastFmConfig.h"
#include "lastfm/ws.h"
#include "lastfm/XmlQuery"

using namespace Tomahawk::Accounts;

LastFmConfig::LastFmConfig( LastFmAccount* account )
    : QWidget( 0 )
    , m_account( account )
{
    m_ui = new Ui_LastFmConfig;
    m_ui->setupUi( this );

    m_ui->username->setText( m_account->username() );
    m_ui->password->setText( m_account->password() );
    m_ui->enable->setChecked( m_account->scrobble() );

    connect( m_ui->testLogin, SIGNAL( clicked( bool ) ), this, SLOT( testLogin( bool ) ) );

    connect( m_ui->username, SIGNAL( textChanged( QString ) ), this, SLOT( enableButton() ) );
    connect( m_ui->password, SIGNAL( textChanged( QString ) ), this, SLOT( enableButton() ) );

// #ifdef Q_WS_MAC // FIXME
//     m_ui->testLogin->setVisible( false );
// #endif
}


QString
LastFmConfig::password() const
{
    return m_ui->password->text();
}


bool
LastFmConfig::scrobble() const
{
    return m_ui->enable->isChecked();
}


QString
LastFmConfig::username() const
{
    return m_ui->username->text().trimmed();
}


void
LastFmConfig::testLogin(bool )
{
    m_ui->testLogin->setEnabled( false );
    m_ui->testLogin->setText( "Testing..." );

    QString authToken = TomahawkUtils::md5( ( m_ui->username->text().toLower() + TomahawkUtils::md5( m_ui->password->text().toUtf8() ) ).toUtf8() );

    // now authenticate w/ last.fm and get our session key
    QMap<QString, QString> query;
    query[ "method" ] = "auth.getMobileSession";
    query[ "username" ] =  m_ui->username->text().toLower();
    query[ "authToken" ] = authToken;

    // ensure they have up-to-date settings
    lastfm::setNetworkAccessManager( TomahawkUtils::nam() );

    QNetworkReply* authJob = lastfm::ws::post( query );

    connect( authJob, SIGNAL( finished() ), SLOT( onLastFmFinished() ) );
}


void
LastFmConfig::enableButton()
{
    m_ui->testLogin->setText( tr( "Test Login" ) );
    m_ui->testLogin->setEnabled( true );
}


void
LastFmConfig::onLastFmFinished()
{
    QNetworkReply* authJob = dynamic_cast<QNetworkReply*>( sender() );
    if( !authJob )
    {
        qDebug() << Q_FUNC_INFO << "No auth job returned!";
        return;
    }
    if( authJob->error() == QNetworkReply::NoError )
    {
        lastfm::XmlQuery lfm = lastfm::XmlQuery( authJob->readAll() );

        if( lfm.children( "error" ).size() > 0 )
        {
            qDebug() << "ERROR from last.fm:" << lfm.text();
            m_ui->testLogin->setText( tr( "Failed" ) );
            m_ui->testLogin->setEnabled( true );
        }
        else
        {
            m_ui->testLogin->setText( tr( "Success" ) );
            m_ui->testLogin->setEnabled( false );
        }
    }
    else
    {
        switch( authJob->error() )
        {
            case QNetworkReply::ContentOperationNotPermittedError:
            case QNetworkReply::AuthenticationRequiredError:
                m_ui->testLogin->setText( tr( "Failed" ) );
                m_ui->testLogin->setEnabled( true );
                break;

            default:
                qDebug() << "Couldn't get last.fm auth result";
                m_ui->testLogin->setText( tr( "Could not contact server" ) );
                m_ui->testLogin->setEnabled( true );
                return;
        }
    }
}
