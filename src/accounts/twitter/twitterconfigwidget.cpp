/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "twitterconfigwidget.h"
#include "twitteraccount.h"
#include "ui_twitterconfigwidget.h"

#include "tomahawksettings.h"
#include "utils/tomahawkutils.h"
#include "database/database.h"

#include "tomahawkoauthtwitter.h"
#include <QTweetLib/qtweetaccountverifycredentials.h>
#include <QTweetLib/qtweetstatusupdate.h>
#include <QTweetLib/qtweetdirectmessagenew.h>

#include <QMessageBox>

#include "utils/logger.h"

namespace Tomahawk
{

namespace Accounts
{

TwitterConfigWidget::TwitterConfigWidget( TwitterAccount* account, QWidget *parent ) :
    QWidget( parent ),
    ui( new Ui::TwitterConfigWidget ),
    m_account( account )
{
    ui->setupUi( this );

    connect( ui->twitterAuthenticateButton, SIGNAL( pressed() ),
            this, SLOT( authDeauthTwitter() ) );
    connect( ui->twitterTweetGotTomahawkButton, SIGNAL( pressed() ),
            this, SLOT( startPostGotTomahawkStatus() ) );
    connect( ui->twitterTweetComboBox, SIGNAL( currentIndexChanged( int ) ),
            this, SLOT( tweetComboBoxIndexChanged( int ) ) );

    ui->twitterTweetComboBox->setCurrentIndex( 0 );
    ui->twitterTweetGotTomahawkButton->setText( tr( "Tweet!" ) );

    QVariantHash credentials = m_account->credentials();
    
    if ( credentials[ "oauthtoken" ].toString().isEmpty() ||
         credentials[ "oauthtokensecret" ].toString().isEmpty() ||
         credentials[ "username" ].toString().isEmpty() )
    {
        ui->twitterStatusLabel->setText( tr( "Status: No saved credentials" ) );
        ui->twitterAuthenticateButton->setText( tr( "Authenticate" ) );
        ui->twitterSyncGroupBox->setVisible( false );

        emit twitterAuthed( false );
    }
    else
    {
        ui->twitterStatusLabel->setText( tr( "Status: Credentials saved for %1" ).arg( m_account->credentials()[ "username" ].toString() ) );
        ui->twitterAuthenticateButton->setText( tr( "De-authenticate" ) );
        ui->twitterSyncGroupBox->setVisible( true );
        ui->twitterUserTweetLineEdit->setVisible( false );

        emit twitterAuthed( true );
    }

}

TwitterConfigWidget::~TwitterConfigWidget()
{
    delete ui;
}

void
TwitterConfigWidget::authDeauthTwitter()
{
    if ( ui->twitterAuthenticateButton->text() == tr( "Authenticate" ) ) //FIXME: don't rely on UI strings here!
        authenticateTwitter();
    else
        deauthenticateTwitter();
}

void
TwitterConfigWidget::authenticateTwitter()
{
    qDebug() << Q_FUNC_INFO;
    TomahawkOAuthTwitter *twitAuth = new TomahawkOAuthTwitter( TomahawkUtils::nam(), this );
    twitAuth->authorizePin();

    QVariantHash credentials = m_account->credentials();
    credentials[ "oauthtoken" ] = twitAuth->oauthToken();
    credentials[ "oauthtokensecret" ] = twitAuth->oauthTokenSecret();
    m_account->setCredentials( credentials );

    QTweetAccountVerifyCredentials *credVerifier = new QTweetAccountVerifyCredentials( twitAuth, this );
    connect( credVerifier, SIGNAL( parsedUser( const QTweetUser & ) ), SLOT( authenticateVerifyReply( const QTweetUser & ) ) );
    connect( credVerifier, SIGNAL( error( QTweetNetBase::ErrorCode, QString ) ), SLOT( authenticateVerifyError( QTweetNetBase::ErrorCode, QString ) ) );
    credVerifier->verify();
}

void
TwitterConfigWidget::authenticateVerifyReply( const QTweetUser &user )
{
    qDebug() << Q_FUNC_INFO;
    if ( user.id() == 0 )
    {
        QMessageBox::critical( this, tr("Tweetin' Error"), tr("The credentials could not be verified.\nYou may wish to try re-authenticating.") );
        emit twitterAuthed( false );
        return;
    }

    QVariantHash credentials = m_account->credentials();
    credentials[ "username" ] = user.screenName();
    m_account->setCredentials( credentials );

    QVariantHash configuration = m_account->configuration();
    configuration[ "sipcachedfriendssinceid" ] = 0;
    configuration[ "sipcachedmentionssinceid" ] = 0;
    m_account->setConfiguration( configuration );
    
    ui->twitterStatusLabel->setText( tr( "Status: Credentials saved for %1" ).arg( user.screenName() ) );
    ui->twitterAuthenticateButton->setText( tr( "De-authenticate" ) );
    ui->twitterSyncGroupBox->setVisible( true );
    ui->twitterTweetComboBox->setCurrentIndex( 0 );
    ui->twitterUserTweetLineEdit->setVisible( false );
    ui->twitterTweetGotTomahawkButton->setText( tr( "Tweet!" ) );

    emit twitterAuthed( true );
    emit sizeHintChanged();
}

void
TwitterConfigWidget::authenticateVerifyError( QTweetNetBase::ErrorCode code, const QString &errorMsg )
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "Error validating credentials, error code is " << code << ", error message is " << errorMsg;
    ui->twitterStatusLabel->setText(tr("Status: Error validating credentials"));
    emit twitterAuthed( false );
    return;
}

void
TwitterConfigWidget::deauthenticateTwitter()
{
    qDebug() << Q_FUNC_INFO;
    QVariantHash credentials = m_account->credentials();
    credentials[ "oauthtoken" ] = QString();
    credentials[ "oauthtokensecret" ] = QString();
    credentials[ "username" ] = QString();
    m_account->setCredentials( credentials );

    ui->twitterStatusLabel->setText(tr("Status: No saved credentials"));
    ui->twitterAuthenticateButton->setText( tr( "Authenticate" ) );
    ui->twitterSyncGroupBox->setVisible( false );

    emit twitterAuthed( false );
    emit sizeHintChanged();
}

void
TwitterConfigWidget::tweetComboBoxIndexChanged( int index )
{
    Q_UNUSED( index );
    if ( ui->twitterTweetComboBox->currentText() == tr( "Global Tweet" ) ) //FIXME: use data!
        ui->twitterUserTweetLineEdit->setVisible( false );
    else
        ui->twitterUserTweetLineEdit->setVisible( true );

    if ( ui->twitterTweetComboBox->currentText() == tr( "Direct Message" ) ) //FIXME: use data!
        ui->twitterTweetGotTomahawkButton->setText( tr( "Send Message!" ) );
    else if ( ui->twitterTweetComboBox->currentText() == tr( "@Mention" ) )
        ui->twitterTweetGotTomahawkButton->setText( tr( "Send Mention!" ) );
    else
        ui->twitterTweetGotTomahawkButton->setText( tr( "Tweet!" ) );
}

void
TwitterConfigWidget::startPostGotTomahawkStatus()
{
    qDebug() << Q_FUNC_INFO;
    m_postGTtype = ui->twitterTweetComboBox->currentText();

    if ( m_postGTtype != "Global Tweet" && ( ui->twitterUserTweetLineEdit->text().isEmpty() || ui->twitterUserTweetLineEdit->text() == "@" ) )
    {
        QMessageBox::critical( this, tr("Tweetin' Error"), tr("You must enter a user name for this type of tweet.") );
        return;
    }

    qDebug() << "Posting Got Tomahawk status";
    QVariantHash credentials = m_account->credentials();
    
    if ( credentials[ "oauthtoken" ].toString().isEmpty() ||
         credentials[ "oauthtokensecret" ].toString().isEmpty() ||
         credentials[ "username" ].toString().isEmpty() )
    {
        QMessageBox::critical( this, tr("Tweetin' Error"), tr("Your saved credentials could not be loaded.\nYou may wish to try re-authenticating.") );
        emit twitterAuthed( false );
        return;
    }
    TomahawkOAuthTwitter *twitAuth = new TomahawkOAuthTwitter( TomahawkUtils::nam(), this );
    twitAuth->setOAuthToken( credentials[ "oauthtoken" ].toString().toLatin1() );
    twitAuth->setOAuthTokenSecret( credentials[ "oauthtokensecret" ].toString().toLatin1() );
    QTweetAccountVerifyCredentials *credVerifier = new QTweetAccountVerifyCredentials( twitAuth, this );
    connect( credVerifier, SIGNAL( parsedUser(const QTweetUser &) ), SLOT( postGotTomahawkStatusAuthVerifyReply(const QTweetUser &) ) );
    credVerifier->verify();
}

void
TwitterConfigWidget::postGotTomahawkStatusAuthVerifyReply( const QTweetUser &user )
{
    qDebug() << Q_FUNC_INFO;
    if ( user.id() == 0 )
    {
        QMessageBox::critical( this, tr("Tweetin' Error"), tr("Your saved credentials could not be verified.\nYou may wish to try re-authenticating.") );
        emit twitterAuthed( false );
        return;
    }
    TomahawkOAuthTwitter *twitAuth = new TomahawkOAuthTwitter( TomahawkUtils::nam(), this );
    QVariantHash credentials = m_account->credentials();
    twitAuth->setOAuthToken( credentials[ "oauthtoken" ].toString().toLatin1() );
    twitAuth->setOAuthTokenSecret( credentials[ "oauthtokensecret" ].toString().toLatin1() );
    if ( m_postGTtype != "Direct Message" )
    {
        QTweetStatusUpdate *statUpdate = new QTweetStatusUpdate( twitAuth, this );
        connect( statUpdate, SIGNAL( postedStatus(const QTweetStatus &) ), SLOT( postGotTomahawkStatusUpdateReply(const QTweetStatus &) ) );
        connect( statUpdate, SIGNAL( error(QTweetNetBase::ErrorCode, const QString&) ), SLOT( postGotTomahawkStatusUpdateError(QTweetNetBase::ErrorCode, const QString &) ) );
        QString uuid = QUuid::createUuid();
        QString message = QString( "Got Tomahawk? {" ) + Database::instance()->dbid() + QString( "} (" ) + uuid.mid( 1, 8 ) + QString( ")" ) + QString( " http://gettomahawk.com" );
        if ( m_postGTtype == "@Mention" )
        {
            QString user = ui->twitterUserTweetLineEdit->text();
            if ( user.startsWith( "@" ) )
                user.remove( 0, 1 );
            message = QString( "@" ) + user + QString( " " ) + message;
        }
        statUpdate->post( message );
    }
    else
    {
        QTweetDirectMessageNew *statUpdate = new QTweetDirectMessageNew( twitAuth, this );
        connect( statUpdate, SIGNAL( parsedDirectMessage(const QTweetDMStatus &)), SLOT( postGotTomahawkDirectMessageReply(const QTweetDMStatus &) ) );
        connect( statUpdate, SIGNAL( error(QTweetNetBase::ErrorCode, const QString&) ), SLOT( postGotTomahawkStatusUpdateError(QTweetNetBase::ErrorCode, const QString &) ) );
        QString uuid = QUuid::createUuid();
        QString message = QString( "Got Tomahawk? {" ) + Database::instance()->dbid() + QString( "} (" ) + uuid.mid( 1, 8 ) + QString( ")" ) + QString( " http://gettomahawk.com" );
        QString user = ui->twitterUserTweetLineEdit->text();
        if ( user.startsWith( "@" ) )
            user.remove( 0, 1 );
        statUpdate->post( user, message );
    }
}

void
TwitterConfigWidget::postGotTomahawkStatusUpdateReply( const QTweetStatus& status )
{
    if ( status.id() == 0 )
        QMessageBox::critical( this, tr("Tweetin' Error"), tr("There was an error posting your status -- sorry!") );
    else
        QMessageBox::information( this, tr("Tweeted!"), tr("Your tweet has been posted!") );
}

void
TwitterConfigWidget::postGotTomahawkDirectMessageReply( const QTweetDMStatus& status )
{
    if ( status.id() == 0 )
        QMessageBox::critical( this, tr("Tweetin' Error"), tr("There was an error posting your direct message -- sorry!") );
    else
        QMessageBox::information( this, tr("Tweeted!"), tr("Your message has been posted!") );
}

void
TwitterConfigWidget::postGotTomahawkStatusUpdateError( QTweetNetBase::ErrorCode code, const QString& errorMsg )
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "Error posting Got Tomahawk message, error code is " << code << ", error message is " << errorMsg;
    QMessageBox::critical( this, tr("Tweetin' Error"), tr("There was an error posting your status -- sorry!") );
}

}

}