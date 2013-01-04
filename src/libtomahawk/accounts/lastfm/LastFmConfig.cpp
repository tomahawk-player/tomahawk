/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
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

#include <boost/bind.hpp>

#include "LastFmAccount.h"
#include "database/Database.h"
#include "database/DatabaseCommand_LogPlayback.h"
#include <database/DatabaseCommand_LoadSocialActions.h>
#include <database/DatabaseCommand_SocialAction.h>
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "utils/Closure.h"

#include <lastfm/ws.h>
#include <lastfm/User.h>
#include <lastfm/XmlQuery.h>
#include <lastfm/Track.h>

using namespace Tomahawk::Accounts;


LastFmConfig::LastFmConfig( LastFmAccount* account )
    : QWidget( 0 )
    , m_account( account )
    , m_page( 1 )
    , m_lastTimeStamp( 0 )
    , m_totalLovedPages( -1 )
    , m_doneFetchingLoved( false )
    , m_doneFetchingLocal( false )
{
    m_ui = new Ui_LastFmConfig;
    m_ui->setupUi( this );

    m_ui->progressBar->hide();

    m_ui->username->setText( m_account->username() );
    m_ui->password->setText( m_account->password() );
    m_ui->enable->setChecked( m_account->scrobble() );

    connect( m_ui->testLogin, SIGNAL( clicked( bool ) ), SLOT( testLogin() ) );
    connect( m_ui->importHistory, SIGNAL( clicked( bool ) ), SLOT( loadHistory() ) );
    connect( m_ui->syncLovedTracks, SIGNAL( clicked( bool ) ), SLOT( syncLovedTracks() ) );

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

    emit sizeHintChanged();

    QNetworkReply* reply = lastfm::User( m_ui->username->text().toLower() ).getRecentTracks( 200, m_page );
    connect( reply, SIGNAL( finished() ), SLOT( onHistoryLoaded() ) );
}


void
LastFmConfig::onHistoryLoaded()
{
    uint total = 0;
    bool finished = false;
    QNetworkReply* reply = qobject_cast< QNetworkReply* >( sender() );

    try
    {
        lastfm::XmlQuery lfm;
        lfm.parse( reply->readAll() );

        foreach ( lastfm::XmlQuery e, lfm.children( "track" ) )
        {
//            tDebug() << "Found:" << e.children( "artist" ).first()["name"].text() << e["name"].text() << e["date"].attribute( "uts" ).toUInt();
            Tomahawk::query_ptr query = Tomahawk::Query::get( e.children( "artist" ).first()["name"].text(), e["name"].text(), QString(), QString(), false );
            if ( query.isNull() )
                continue;

            m_lastTimeStamp = e["date"].attribute( "uts" ).toUInt();

            DatabaseCommand_LogPlayback* cmd = new DatabaseCommand_LogPlayback( query, DatabaseCommand_LogPlayback::Finished, m_lastTimeStamp );
            Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
        }

        if ( !lfm.children( "recenttracks" ).isEmpty() )
        {
            lastfm::XmlQuery stats = lfm.children( "recenttracks" ).first();

            uint page = stats.attribute( "page" ).toUInt();
            total = stats.attribute( "totalPages" ).toUInt();

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
        tDebug() << "XmlQuery error:" << e.message();
        finished = true;
    }

    if ( finished )
    {
        if ( m_page != total )
        {
            //: Text on a button that resumes import
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
        lastfm::XmlQuery lfm;
        lfm.parse( authJob->readAll() );

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
            m_ui->syncLovedTracks->setEnabled( true );
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


void
LastFmConfig::syncLovedTracks( uint page )
{
    QNetworkReply* reply = lastfm::User( username() ).getLovedTracks( 200, page );

    m_ui->syncLovedTracks->setEnabled( false );
    m_ui->syncLovedTracks->setText( tr( "Synchronizing..." ) );
    m_ui->progressBar->show();
    emit sizeHintChanged();

    NewClosure( reply, SIGNAL( finished() ), this, SLOT( onLovedFinished( QNetworkReply* ) ), reply );

    DatabaseCommand_LoadSocialActions* cmd = new DatabaseCommand_LoadSocialActions( "Love", SourceList::instance()->getLocal() );
    connect( cmd, SIGNAL( done( DatabaseCommand_LoadSocialActions::TrackActions ) ), this, SLOT( localLovedLoaded( DatabaseCommand_LoadSocialActions::TrackActions ) ) );

    Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );
}


void
LastFmConfig::onLovedFinished( QNetworkReply* reply )
{
    Q_ASSERT( reply );

    try
    {
        lastfm::XmlQuery lfm;
        lfm.parse( reply->readAll() );

        if ( !lfm.children( "lovedtracks" ).isEmpty() )
        {
            lastfm::XmlQuery loved = lfm.children( "lovedtracks" ).first();

            const int thisPage = loved.attribute( "page" ).toInt();

            if ( m_totalLovedPages < 0 )
            {
                m_totalLovedPages = loved.attribute( "totalPages" ).toInt();
                m_ui->progressBar->setMaximum( m_totalLovedPages + 2 );
            }

            m_ui->progressBar->setValue( thisPage );
            foreach ( lastfm::XmlQuery e, loved.children( "track" ) )
            {
                 tDebug() << "Found:" << e.children( "artist" ).first()["name"].text() << e["name"].text() << e["date"].attribute( "uts" ).toUInt();
                Tomahawk::query_ptr query = Tomahawk::Query::get( e.children( "artist" ).first()["name"].text(), e["name"].text(), QString(), QString(), false );
                if ( query.isNull() )
                    continue;

                m_lastfmLoved.insert( query );
            }

            if ( thisPage == m_totalLovedPages )
            {
                m_doneFetchingLoved = true;

                if ( m_doneFetchingLocal )
                    syncLoved();

                return;
            }
            else
            {
                syncLovedTracks( thisPage + 1 );
            }
        }
        else
        {
            m_ui->syncLovedTracks->setText( "Failed" );
            m_ui->progressBar->hide();
            emit sizeHintChanged();
        }
    }
    catch( lastfm::ws::ParseError e )
    {
        m_ui->syncLovedTracks->setText( "Failed" );
        m_ui->progressBar->hide();
        emit sizeHintChanged();
        tDebug() << "XmlQuery error:" << e.message();
    }
}


bool trackEquality( const Tomahawk::query_ptr& first, const Tomahawk::query_ptr& second )
{
    qDebug() << "Comparing:" << first->track() << second->track();
    qDebug() << "==========" << first->artist() << second->artist();
    return first->equals( second, true );
}


void
LastFmConfig::localLovedLoaded( DatabaseCommand_LoadSocialActions::TrackActions tracks )
{
    m_localLoved = tracks;
    m_doneFetchingLocal = true;

    if ( m_doneFetchingLoved )
        syncLoved();
}


void
LastFmConfig::syncLoved()
{
    QSet< Tomahawk::query_ptr > localToLove, lastFmToLove, lastFmToUnlove;

    const QSet< Tomahawk::query_ptr > myLoved = m_localLoved.keys().toSet();

    m_ui->progressBar->setValue( m_ui->progressBar->value() + 1 );

    foreach ( const Tomahawk::query_ptr& lastfmLoved, m_lastfmLoved )
    {
        QSet< Tomahawk::query_ptr >::const_iterator iter = std::find_if( myLoved.begin(), myLoved.end(), boost::bind( &trackEquality, _1, boost::ref( lastfmLoved ) ) );
        if ( iter == myLoved.constEnd() )
        {
//             qDebug() << "Found last.fm loved track that we didn't have loved locally:" << lastfmLoved->track() << lastfmLoved->artist();
            localToLove << lastfmLoved;
        }
    }

    foreach ( const Tomahawk::query_ptr& localLoved, myLoved )
    {
        qDebug() << "CHECKING FOR LOCAL LOVED ON LAST.FM TOO:" << m_localLoved[ localLoved ].value.toString() << localLoved->track() << localLoved->artist();
        QSet< Tomahawk::query_ptr >::const_iterator iter = std::find_if( m_lastfmLoved.begin(), m_lastfmLoved.end(), boost::bind( &trackEquality, _1, boost::ref( localLoved ) ) );

        qDebug() << "Result:" << (iter == m_lastfmLoved.constEnd());
        // If we unloved it locally, but it's still loved on last.fm, unlove it
        if ( m_localLoved[ localLoved ].value.toString() == "false" && iter != m_lastfmLoved.constEnd() )
            lastFmToUnlove << localLoved;

        // If we loved it locally but not loved on last.fm, love it
        if ( m_localLoved[ localLoved ].value.toString() == "true" && iter == m_lastfmLoved.constEnd() )
        {
            qDebug() << "Found Local loved track but not on last.fm!:" << localLoved->track() << localLoved->artist();
            lastFmToLove << localLoved;
        }
    }

    foreach ( const Tomahawk::query_ptr& track, localToLove )
    {
        // Don't use the infosystem as we don't want to tweet a few hundred times :)
        DatabaseCommand_SocialAction* cmd = new DatabaseCommand_SocialAction( track, QString( "Love" ), QString( "true" ) );
        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
    }

    lastFmToLove.unite( lastFmToUnlove );

    foreach ( const Tomahawk::query_ptr& track, lastFmToLove )
    {
        lastfm::MutableTrack lfmTrack;
        lfmTrack.stamp();

        lfmTrack.setTitle( track->track() );
        lfmTrack.setArtist( track->artist() );
        lfmTrack.setSource( lastfm::Track::Player );

        if ( lastFmToUnlove.contains( track ) )
            lfmTrack.unlove();
        else
            lfmTrack.love();
    }

    m_ui->progressBar->setValue( m_ui->progressBar->value() + 1 );
    m_ui->syncLovedTracks->setText( tr( "Synchronization Finished" ) );
}

