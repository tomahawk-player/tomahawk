/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "TwitterConfigWidget.h"
#include "TwitterAccount.h"
#include "ui_TwitterConfigWidget.h"

#include "TomahawkSettings.h"
#include "utils/TomahawkUtils.h"
#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "Source.h"
#include "TomahawkOAuthTwitter.h"

#include <QTweetLib/qtweetaccountverifycredentials.h>
#include <QTweetLib/qtweetstatusupdate.h>
#include <QTweetLib/qtweetdirectmessagenew.h>

#include <QMessageBox>

#include "utils/Logger.h"

namespace Tomahawk
{

namespace Accounts
{

TwitterConfigWidget::TwitterConfigWidget( TwitterAccount* account, QWidget *parent ) :
    AccountConfigWidget( parent ),
    m_ui( new Ui::TwitterConfigWidget ),
    m_account( account )
{
    m_ui->setupUi( this );

    connect( m_ui->twitterAuthenticateButton, SIGNAL( pressed() ),
            this, SLOT( authDeauthTwitter() ) );
    connect( m_ui->twitterTweetGotTomahawkButton, SIGNAL( pressed() ),
            this, SLOT( startPostGotTomahawkStatus() ) );
    connect( m_ui->twitterTweetComboBox, SIGNAL( currentIndexChanged( int ) ),
            this, SLOT( tweetComboBoxIndexChanged( int ) ) );

    m_ui->twitterTweetComboBox->setCurrentIndex( 0 );
    m_ui->twitterTweetGotTomahawkButton->setText( tr( "Tweet!" ) );

    QVariantHash credentials = m_account->credentials();

    if ( credentials[ "oauthtoken" ].toString().isEmpty() ||
         credentials[ "oauthtokensecret" ].toString().isEmpty() ||
         credentials[ "username" ].toString().isEmpty() )
    {
        m_ui->twitterStatusLabel->setText( tr( "Status: No saved credentials" ) );
        m_ui->twitterAuthenticateButton->setText( tr( "Authenticate" ) );
        m_ui->twitterSyncGroupBox->setVisible( false );

        emit twitterAuthed( false );
    }
    else
    {
        m_ui->twitterStatusLabel->setText( tr( "Status: Credentials saved for %1" ).arg( m_account->credentials()[ "username" ].toString() ) );
        m_ui->twitterAuthenticateButton->setText( tr( "De-authenticate" ) );
        //m_ui->twitterSyncGroupBox->setVisible( true );
        m_ui->twitterUserTweetLineEdit->setVisible( false );

        emit twitterAuthed( true );
    }

    m_ui->twitterSyncGroupBox->hide();
}

TwitterConfigWidget::~TwitterConfigWidget()
{
    delete m_ui;
}

void
TwitterConfigWidget::authDeauthTwitter()
{
    if ( m_ui->twitterAuthenticateButton->text() == tr( "Authenticate" ) ) //FIXME: don't rely on UI strings here!
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

    m_ui->twitterStatusLabel->setText( tr( "Status: Credentials saved for %1" ).arg( user.screenName() ) );
    m_ui->twitterAuthenticateButton->setText( tr( "De-authenticate" ) );
    //m_ui->twitterSyncGroupBox->setVisible( true );
    m_ui->twitterTweetComboBox->setCurrentIndex( 0 );
    m_ui->twitterUserTweetLineEdit->setVisible( false );
    m_ui->twitterTweetGotTomahawkButton->setText( tr( "Tweet!" ) );

    emit twitterAuthed( true );
    emit sizeHintChanged();
}

void
TwitterConfigWidget::authenticateVerifyError( QTweetNetBase::ErrorCode code, const QString &errorMsg )
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "Error validating credentials, error code is " << code << ", error message is " << errorMsg;
    m_ui->twitterStatusLabel->setText(tr("Status: Error validating credentials"));
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

    m_ui->twitterStatusLabel->setText(tr("Status: No saved credentials"));
    m_ui->twitterAuthenticateButton->setText( tr( "Authenticate" ) );
    m_ui->twitterSyncGroupBox->setVisible( false );

    emit twitterAuthed( false );
    emit sizeHintChanged();
}

void
TwitterConfigWidget::tweetComboBoxIndexChanged( int index )
{
    Q_UNUSED( index );
    if ( m_ui->twitterTweetComboBox->currentText() == tr( "Global Tweet" ) ) //FIXME: use data!
        m_ui->twitterUserTweetLineEdit->setVisible( false );
    else
        m_ui->twitterUserTweetLineEdit->setVisible( true );

    if ( m_ui->twitterTweetComboBox->currentText() == tr( "Direct Message" ) ) //FIXME: use data!
        m_ui->twitterTweetGotTomahawkButton->setText( tr( "Send Message!" ) );
    else if ( m_ui->twitterTweetComboBox->currentText() == tr( "@Mention" ) )
        m_ui->twitterTweetGotTomahawkButton->setText( tr( "Send Mention!" ) );
    else
        m_ui->twitterTweetGotTomahawkButton->setText( tr( "Tweet!" ) );
}

void
TwitterConfigWidget::startPostGotTomahawkStatus()
{
    qDebug() << Q_FUNC_INFO;
    m_postGTtype = m_ui->twitterTweetComboBox->currentText();

    if ( m_postGTtype != "Global Tweet" && ( m_ui->twitterUserTweetLineEdit->text().isEmpty() || m_ui->twitterUserTweetLineEdit->text() == "@" ) )
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
        QString message = QString( "Got Tomahawk? {" ) + Database::instance()->impl()->dbid() + QString( "} (" ) + uuid.mid( 1, 8 ) + QString( ")" ) + QString( " http://gettomahawk.com" );
        if ( m_postGTtype == "@Mention" )
        {
            QString user = m_ui->twitterUserTweetLineEdit->text();
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
        QString message = QString( "Got Tomahawk? {" ) + Database::instance()->impl()->dbid() + QString( "} (" ) + uuid.mid( 1, 8 ) + QString( ")" ) + QString( " http://gettomahawk.com" );
        QString user = m_ui->twitterUserTweetLineEdit->text();
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
