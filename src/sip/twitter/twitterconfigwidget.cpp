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
#include "ui_twitterconfigwidget.h"

#include "tomahawksettings.h"
#include "utils/tomahawkutils.h"
#include "database/database.h"

#include "tomahawkoauthtwitter.h"
#include <qtweetaccountverifycredentials.h>
#include <qtweetstatusupdate.h>

#include <QMessageBox>

TwitterConfigWidget::TwitterConfigWidget(SipPlugin* plugin, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TwitterConfigWidget),
    m_plugin(plugin)
{
    ui->setupUi(this);

    connect(ui->twitterAuthenticateButton, SIGNAL(pressed()),
            this,   SLOT(authenticateTwitter()));
    connect(ui->twitterTweetGotTomahawkButton, SIGNAL(pressed()),
            this,   SLOT(startPostGotTomahawkStatus()));


    TomahawkSettings* s = TomahawkSettings::instance();
    if ( s->twitterOAuthToken().isEmpty() || s->twitterOAuthTokenSecret().isEmpty() )
    {
        ui->twitterStatusLabel->setText("Status: No saved credentials");
        ui->twitterAuthenticateButton->setText( "Authenticate" );
        ui->twitterInstructionsBox->setVisible( false );
    }
    else
    {
        ui->twitterStatusLabel->setText("Status: Credentials saved");
        ui->twitterAuthenticateButton->setText( "Re-authenticate" );
        ui->twitterInstructionsBox->setVisible( true );
    }

}

TwitterConfigWidget::~TwitterConfigWidget()
{
    delete ui;
}

void
TwitterConfigWidget::authenticateTwitter()
{
    qDebug() << Q_FUNC_INFO;
    TomahawkOAuthTwitter *twitAuth = new TomahawkOAuthTwitter( this );
    twitAuth->setNetworkAccessManager( TomahawkUtils::nam() );
    twitAuth->authorizePin();
    if ( !twitAuth->oauthToken().isEmpty() && !twitAuth->oauthTokenSecret().isEmpty() )
    {
        TomahawkSettings* s = TomahawkSettings::instance();
        s->setTwitterOAuthToken( twitAuth->oauthToken() );
        s->setTwitterOAuthTokenSecret( twitAuth->oauthTokenSecret() );
        ui->twitterStatusLabel->setText("Status: Credentials saved");
        ui->twitterAuthenticateButton->setText( "Re-authenticate" );
        ui->twitterInstructionsBox->setVisible( true );
        TomahawkSettings::instance()->setTwitterCachedFriendsSinceId( 0 );
        TomahawkSettings::instance()->setTwitterCachedMentionsSinceId( 0 );
        m_plugin->connectPlugin( false );

    }
    else
    {
        TomahawkSettings* s = TomahawkSettings::instance();
        s->setTwitterOAuthToken( QString() );
        s->setTwitterOAuthTokenSecret( QString() );
        ui->twitterStatusLabel->setText("Status: No saved credentials");
        ui->twitterAuthenticateButton->setText( "Authenticate" );
        ui->twitterInstructionsBox->setVisible( false );
        QMessageBox::critical( 0, QString("Tweetin' Error"), QString("There was an error validating your authentication") );
    }
}


void
TwitterConfigWidget::startPostGotTomahawkStatus()
{
    qDebug() << "Posting Got Tomahawk status";
    TomahawkSettings* s = TomahawkSettings::instance();
    if ( s->twitterOAuthToken().isEmpty() || s->twitterOAuthTokenSecret().isEmpty() )
    {
        QMessageBox::critical( 0, QString("Tweetin' Error"), QString("Your saved credentials could not be loaded.\nYou may wish to try re-authenticating.") );
        return;
    }
    TomahawkOAuthTwitter *twitAuth = new TomahawkOAuthTwitter( this );
    twitAuth->setNetworkAccessManager( TomahawkUtils::nam() );
    twitAuth->setOAuthToken( s->twitterOAuthToken().toLatin1() );
    twitAuth->setOAuthTokenSecret( s->twitterOAuthTokenSecret().toLatin1() );
    QTweetAccountVerifyCredentials *credVerifier = new QTweetAccountVerifyCredentials( twitAuth, this );
    connect( credVerifier, SIGNAL( parsedUser(const QTweetUser &) ), SLOT( postGotTomahawkStatusAuthVerifyReply(const QTweetUser &) ) );
    credVerifier->verify();
}


void
TwitterConfigWidget::postGotTomahawkStatusAuthVerifyReply( const QTweetUser &user )
{
    if ( user.id() == 0 )
    {
        QMessageBox::critical( 0, QString("Tweetin' Error"), QString("Your saved credentials could not be verified.\nYou may wish to try re-authenticating.") );
        return;
    }
    TomahawkSettings* s = TomahawkSettings::instance();
    s->setTwitterScreenName( user.screenName() );
    TomahawkOAuthTwitter *twitAuth = new TomahawkOAuthTwitter( this );
    twitAuth->setNetworkAccessManager( TomahawkUtils::nam() );
    twitAuth->setOAuthToken( s->twitterOAuthToken().toLatin1() );
    twitAuth->setOAuthTokenSecret( s->twitterOAuthTokenSecret().toLatin1() );
    QTweetStatusUpdate *statUpdate = new QTweetStatusUpdate( twitAuth, this );
    connect( statUpdate, SIGNAL( postedStatus(const QTweetStatus &) ), SLOT( postGotTomahawkStatusUpdateReply(const QTweetStatus &) ) );
    connect( statUpdate, SIGNAL( error(QTweetNetBase::ErrorCode, const QString&) ), SLOT( postGotTomahawkStatusUpdateError(QTweetNetBase::ErrorCode, const QString &) ) );
    QString uuid = QUuid::createUuid();
    statUpdate->post( QString( "Got Tomahawk? {" ) + Database::instance()->dbid() + QString( "} (" ) + uuid.mid( 1, 8 ) + QString( ")" ) + QString( " http://gettomahawk.com" ) );
}


void
TwitterConfigWidget::postGotTomahawkStatusUpdateReply( const QTweetStatus& status )
{
    if ( status.id() == 0 )
        QMessageBox::critical( 0, QString("Tweetin' Error"), QString("There was an error posting your status -- sorry!") );
    else
        QMessageBox::information( 0, QString("Tweeted!"), QString("Your tweet has been posted!") );
}


void
TwitterConfigWidget::postGotTomahawkStatusUpdateError( QTweetNetBase::ErrorCode code, const QString& errorMsg )
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "Error posting Got Tomahawk message, error code is " << code << ", error message is " << errorMsg;
    QMessageBox::critical( 0, QString("Tweetin' Error"), QString("There was an error posting your status -- sorry!") );
}
