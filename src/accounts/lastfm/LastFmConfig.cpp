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
#include "ui_LastFmConfig.h"

#include "LastFmAccount.h"
#include "database/Database.h"
#include "database/DatabaseCommand_LogPlayback.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "lastfm/ws.h"
#include "lastfm/User"
#include "lastfm/XmlQuery"

using namespace Tomahawk::Accounts;


LastFmConfig::LastFmConfig( LastFmAccount* account )
    : QWidget( 0 )
    , m_account( account )
    , m_page( 1 )
    , m_lastTimeStamp( 0 )
{
    m_ui = new Ui_LastFmConfig;
    m_ui->setupUi( this );

    m_ui->progressBar->hide();

    m_ui->username->setText( m_account->username() );
    m_ui->password->setText( m_account->password() );
    m_ui->enable->setChecked( m_account->scrobble() );

    connect( m_ui->testLogin, SIGNAL( clicked( bool ) ), SLOT( testLogin() ) );
    connect( m_ui->importHistory, SIGNAL( clicked( bool ) ), SLOT( loadHistory() ) );

    connect( m_ui->username, SIGNAL( textChanged( QString ) ), SLOT( enableButton() ) );
    connect( m_ui->password, SIGNAL( textChanged( QString ) ), SLOT( enableButton() ) );
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
LastFmConfig::testLogin()
{
    m_ui->testLogin->setEnabled( false );
    m_ui->testLogin->setText( tr( "Testing..." ) );

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
LastFmConfig::loadHistory()
{
    if ( m_lastTimeStamp )
    {
        m_ui->importHistory->setText( tr( "Importing %1", "e.g. Importing 2012/01/01" ).arg( QDateTime::fromTime_t( m_lastTimeStamp ).toString( "MMMM d yyyy" ) ) );
    }
    else
        m_ui->importHistory->setText( tr( "Importing History..." ) );

    m_ui->importHistory->setEnabled( false );
    m_ui->progressBar->show();

    QNetworkReply* reply = lastfm::User( m_ui->username->text().toLower() ).getRecentTracks( 200, m_page );
    connect( reply, SIGNAL( finished() ), SLOT( onHistoryLoaded() ) );
}


void
LastFmConfig::onHistoryLoaded()
{
    int total = 0;
    bool finished = false;
    QNetworkReply* reply = qobject_cast< QNetworkReply* >( sender() );
    
    try
    {
        lastfm::XmlQuery lfm = reply->readAll();

        foreach ( lastfm::XmlQuery e, lfm.children( "track" ) )
        {
//            tDebug() << "Found:" << e["artist"].text() << e["name"].text() << e["date"].attribute( "uts" ).toUInt();
            Tomahawk::query_ptr query = Query::get( e["artist"].text(), e["name"].text(), QString(), QString(), false );
            m_lastTimeStamp = e["date"].attribute( "uts" ).toUInt();
            
            DatabaseCommand_LogPlayback* cmd = new DatabaseCommand_LogPlayback( query, DatabaseCommand_LogPlayback::Finished, m_lastTimeStamp );
            Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
        }
        
        if ( !lfm.children( "recenttracks" ).isEmpty() )
        {
            lastfm::XmlQuery stats = lfm.children( "recenttracks" ).first();
            
            int page = stats.attribute( "page" ).toInt();
            total = stats.attribute( "totalPages" ).toInt();
            
            m_ui->progressBar->setMaximum( total );
            m_ui->progressBar->setValue( page );
            
            if ( page < total )
            {
                m_page = page + 1;
                loadHistory();
            }
            else
                finished = true;
        }
        else
            finished = true;
    }
    catch( lastfm::ws::ParseError e )
    {
        tDebug() << "XmlQuery error:" << e.what();
        finished = true;
    }
    
    if ( finished )
    {
        if ( m_page != total )
        {
            m_ui->importHistory->setText( tr( "History Incomplete. Resume" ) );
            m_ui->importHistory->setEnabled( true );
        }
        else
        {
            m_ui->importHistory->setText( tr( "Playback History Imported" ) );
        }
    }
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
