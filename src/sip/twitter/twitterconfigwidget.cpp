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
#include "twitter.h"
#include "ui_twitterconfigwidget.h"

#include "tomahawksettings.h"
#include "utils/tomahawkutils.h"
#include "database/database.h"

#include "tomahawkoauthtwitter.h"
#include <qtweetaccountverifycredentials.h>
#include <qtweetstatusupdate.h>
#include <qtweetdirectmessagenew.h>

#include <QMessageBox>

TwitterConfigWidget::TwitterConfigWidget( TwitterPlugin* plugin, QWidget *parent ) :
    QWidget( parent ),
    ui( new Ui::TwitterConfigWidget ),
    m_plugin( plugin )
{
    ui->setupUi( this );

    connect( ui->twitterAuthenticateButton, SIGNAL( pressed() ),
            this, SLOT( authDeauthTwitter() ) );
    connect( ui->twitterTweetGotTomahawkButton, SIGNAL( pressed() ),
            this, SLOT( startPostGotTomahawkStatus() ) );
    connect( ui->twitterTweetComboBox, SIGNAL( currentIndexChanged( int ) ),
            this, SLOT( tweetComboBoxIndexChanged( int ) ) );
    connect( ui->autoConnectCheckbox, SIGNAL( toggled( bool ) ),
            this, SLOT( autoConnectToggled( bool ) ) );


    ui->twitterTweetComboBox->setCurrentIndex( 0 );
    ui->twitterUserTweetLineEdit->setReadOnly( true );
    ui->twitterUserTweetLineEdit->setEnabled( false );

    if ( m_plugin->twitterOAuthToken().isEmpty() || m_plugin->twitterOAuthTokenSecret().isEmpty() || m_plugin->twitterScreenName().isEmpty() )
    {
        ui->twitterStatusLabel->setText( tr( "Status: No saved credentials" ) );
        ui->twitterAuthenticateButton->setText( tr( "Authenticate" ) );
        ui->twitterInstructionsInfoLabel->setVisible( false );
        ui->twitterGlobalTweetLabel->setVisible( false );
        ui->twitterTweetGotTomahawkButton->setVisible( false );
        ui->twitterUserTweetLineEdit->setVisible( false );
        ui->twitterTweetComboBox->setVisible( false );

        emit twitterAuthed( false );
    }
    else
    {
        ui->twitterStatusLabel->setText( tr( "Status: Credentials saved for %1" ).arg( m_plugin->twitterScreenName() ) );
        ui->twitterAuthenticateButton->setText( tr( "De-authenticate" ) );
        ui->twitterInstructionsInfoLabel->setVisible( true );
        ui->twitterGlobalTweetLabel->setVisible( true );
        ui->twitterTweetGotTomahawkButton->setVisible( true );
        ui->twitterUserTweetLineEdit->setVisible( true );
        ui->twitterTweetComboBox->setVisible( true );

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

    m_plugin->setTwitterOAuthToken( twitAuth->oauthToken() );
    m_plugin->setTwitterOAuthTokenSecret( twitAuth->oauthTokenSecret() );

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

    m_plugin->setTwitterScreenName( user.screenName() );
    m_plugin->setTwitterCachedFriendsSinceId( 0 );
    m_plugin->setTwitterCachedMentionsSinceId( 0 );

    ui->twitterStatusLabel->setText( tr( "Status: Credentials saved for %1" ).arg( m_plugin->twitterScreenName() ) );
    ui->twitterAuthenticateButton->setText( tr( "De-authenticate" ) );
    ui->twitterInstructionsInfoLabel->setVisible( true );
    ui->twitterGlobalTweetLabel->setVisible( true );
    ui->twitterTweetGotTomahawkButton->setVisible( true );
    ui->twitterUserTweetLineEdit->setVisible( true );
    ui->twitterTweetComboBox->setVisible( true );

    m_plugin->connectPlugin( false );

    emit twitterAuthed( true );
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
    m_plugin->setTwitterOAuthToken( QString() );
    m_plugin->setTwitterOAuthTokenSecret( QString() );
    m_plugin->setTwitterScreenName( QString() );

    ui->twitterStatusLabel->setText(tr("Status: No saved credentials"));
    ui->twitterAuthenticateButton->setText( tr( "Authenticate" ) );
    ui->twitterInstructionsInfoLabel->setVisible( false );
    ui->twitterGlobalTweetLabel->setVisible( false );
    ui->twitterTweetGotTomahawkButton->setVisible( false );
    ui->twitterUserTweetLineEdit->setVisible( false );
    ui->twitterTweetComboBox->setVisible( false );

    emit twitterAuthed( false );
}

void
TwitterConfigWidget::tweetComboBoxIndexChanged( int index )
{
    Q_UNUSED( index );
    if( ui->twitterTweetComboBox->currentText() == tr( "Global Tweet" ) ) //FIXME: use data!
    {
        ui->twitterUserTweetLineEdit->setReadOnly( true );
        ui->twitterUserTweetLineEdit->setEnabled( false );
    }
    else
    {
        ui->twitterUserTweetLineEdit->setReadOnly( false );
        ui->twitterUserTweetLineEdit->setEnabled( true );
    }

    if( ui->twitterTweetComboBox->currentText() == tr( "Direct Message" ) ) //FIXME: use data!
        ui->twitterTweetGotTomahawkButton->setText( tr( "Send Message!" ) );
    else
        ui->twitterTweetGotTomahawkButton->setText( tr( "Tweet!" ) );
}

void
TwitterConfigWidget::autoConnectToggled( bool on )
{
    m_plugin->setTwitterAutoConnect( on );
}


void
TwitterConfigWidget::startPostGotTomahawkStatus()
{
    m_postGTtype = ui->twitterTweetComboBox->currentText();

    if ( m_postGTtype != "Global Tweet" && ( ui->twitterUserTweetLineEdit->text().isEmpty() || ui->twitterUserTweetLineEdit->text() == "@" ) )
    {
        QMessageBox::critical( this, tr("Tweetin' Error"), tr("You must enter a user name for this type of tweet.") );
        return;
    }

    qDebug() << "Posting Got Tomahawk status";
    if ( m_plugin->twitterOAuthToken().isEmpty() || m_plugin->twitterOAuthTokenSecret().isEmpty() || m_plugin->twitterScreenName().isEmpty() )
    {
        QMessageBox::critical( this, tr("Tweetin' Error"), tr("Your saved credentials could not be loaded.\nYou may wish to try re-authenticating.") );
        emit twitterAuthed( false );
        return;
    }
    TomahawkOAuthTwitter *twitAuth = new TomahawkOAuthTwitter( TomahawkUtils::nam(), this );
    twitAuth->setOAuthToken( m_plugin->twitterOAuthToken().toLatin1() );
    twitAuth->setOAuthTokenSecret( m_plugin->twitterOAuthTokenSecret().toLatin1() );
    QTweetAccountVerifyCredentials *credVerifier = new QTweetAccountVerifyCredentials( twitAuth, this );
    connect( credVerifier, SIGNAL( parsedUser(const QTweetUser &) ), SLOT( postGotTomahawkStatusAuthVerifyReply(const QTweetUser &) ) );
    credVerifier->verify();
}

void
TwitterConfigWidget::postGotTomahawkStatusAuthVerifyReply( const QTweetUser &user )
{
    if ( user.id() == 0 )
    {
        QMessageBox::critical( this, tr("Tweetin' Error"), tr("Your saved credentials could not be verified.\nYou may wish to try re-authenticating.") );
        emit twitterAuthed( false );
        return;
    }
    m_plugin->setTwitterScreenName( user.screenName() );
    TomahawkOAuthTwitter *twitAuth = new TomahawkOAuthTwitter( TomahawkUtils::nam(), this );
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
