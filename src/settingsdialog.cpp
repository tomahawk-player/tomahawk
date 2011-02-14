#include <QCryptographicHash>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QNetworkProxy>

#ifndef NO_LIBLASTFM
#include <lastfm/ws.h>
#include <lastfm/XmlQuery>
#endif

#include <qtweetaccountverifycredentials.h>
#include <qtweetstatusupdate.h>

#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "ui_proxydialog.h"
#include "tomahawk/tomahawkapp.h"
#include "musicscanner.h"
#include "tomahawksettings.h"
#include "sip/SipHandler.h"
#include "sip/twitter/tomahawkoauthtwitter.h"
#include <database/database.h>

static QString
md5( const QByteArray& src )
{
    QByteArray const digest = QCryptographicHash::hash( src, QCryptographicHash::Md5 );
    return QString::fromLatin1( digest.toHex() ).rightJustified( 32, '0' );
}


SettingsDialog::SettingsDialog( QWidget *parent )
    : QDialog( parent )
    , ui( new Ui::SettingsDialog )
    , m_proxySettings( this )
    , m_rejected( false )
    , m_testLastFmQuery( 0 )
{
    ui->setupUi( this );
    TomahawkSettings* s = TomahawkSettings::instance();

    ui->checkBoxHttp->setChecked( s->httpEnabled() );
    ui->checkBoxStaticPreferred->setChecked( s->preferStaticHostPort() );
    ui->checkBoxUpnp->setChecked( s->externalAddressMode() == TomahawkSettings::Upnp );
    ui->checkBoxUpnp->setEnabled( !s->preferStaticHostPort() );

    // JABBER
    ui->checkBoxJabberAutoConnect->setChecked( s->jabberAutoConnect() );
    ui->jabberUsername->setText( s->jabberUsername() );
    ui->jabberPassword->setText( s->jabberPassword() );
    ui->jabberServer->setText( s->jabberServer() );
    ui->jabberPort->setValue( s->jabberPort() );
    
    ui->staticHostName->setText( s->externalHostname() );
    ui->staticPort->setValue( s->externalPort() );

    ui->proxyButton->setVisible( false );
    
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

    // MUSIC SCANNER
    ui->lineEditMusicPath->setText( s->scannerPath() );

    // LAST FM
    ui->checkBoxEnableLastfm->setChecked( s->scrobblingEnabled() );
    ui->lineEditLastfmUsername->setText( s->lastFmUsername() );
    ui->lineEditLastfmPassword->setText(s->lastFmPassword() );
    connect( ui->pushButtonTestLastfmLogin, SIGNAL( clicked( bool) ), this, SLOT( testLastFmLogin() ) );
    
    // SCRIPT RESOLVER
    ui->removeScript->setEnabled( false );
    foreach( const QString& resolver, s->scriptResolvers() ) {
        QFileInfo info( resolver );
        ui->scriptList->addTopLevelItem( new QTreeWidgetItem( QStringList() << info.baseName() << resolver ) );
        
    }
    connect( ui->scriptList, SIGNAL( itemClicked( QTreeWidgetItem*, int ) ), this, SLOT( scriptSelectionChanged() ) );
    connect( ui->addScript, SIGNAL( clicked( bool ) ), this, SLOT( addScriptResolver() ) );
    connect( ui->removeScript, SIGNAL( clicked( bool ) ), this, SLOT( removeScriptResolver() ) );
    
    connect( ui->buttonBrowse, SIGNAL( clicked() ),  SLOT( showPathSelector() ) );
    connect( ui->proxyButton,  SIGNAL( clicked() ),  SLOT( showProxySettings() ) );
    connect( ui->checkBoxStaticPreferred, SIGNAL( toggled(bool) ), SLOT( toggleUpnp(bool) ) );
    connect( this,             SIGNAL( rejected() ), SLOT( onRejected() ) );
}


SettingsDialog::~SettingsDialog()
{
    qDebug() << Q_FUNC_INFO;

    if ( !m_rejected )
    {
        TomahawkSettings* s = TomahawkSettings::instance();

        // if jabber or scan dir changed, reconnect/rescan
        bool rescan = ui->lineEditMusicPath->text() != s->scannerPath();
        bool rejabber = false;
        if ( ui->jabberUsername->text() != s->jabberUsername() ||
             ui->jabberPassword->text() != s->jabberPassword() ||
             ui->jabberServer->text() != s->jabberServer() ||
             (uint)(ui->jabberPort->value()) != s->jabberPort()
             )
            {
            rejabber = true;
        }

        s->setHttpEnabled(                                  ui->checkBoxHttp->checkState() == Qt::Checked );

        s->setPreferStaticHostPort( ui->checkBoxStaticPreferred->checkState() == Qt::Checked );
        s->setExternalAddressMode( ui->checkBoxUpnp->checkState() == Qt::Checked ? TomahawkSettings::Upnp : TomahawkSettings::Lan );

        s->setJabberAutoConnect(                            ui->checkBoxJabberAutoConnect->checkState() == Qt::Checked );
        s->setJabberUsername(                               ui->jabberUsername->text() );
        s->setJabberPassword(                               ui->jabberPassword->text() );
        s->setJabberServer(                                 ui->jabberServer->text() );
        s->setJabberPort(                                   ui->jabberPort->value() );
        
        s->setExternalHostname(                             ui->staticHostName->text() );
        s->setExternalPort(                                 ui->staticPort->value() );

        s->setScannerPath(                                  ui->lineEditMusicPath->text() );
        
        s->setScrobblingEnabled(                            ui->checkBoxEnableLastfm->isChecked() );
        s->setLastFmUsername(                               ui->lineEditLastfmUsername->text() );
        s->setLastFmPassword(                               ui->lineEditLastfmPassword->text() );

        QStringList resolvers;
        for( int i = 0; i < ui->scriptList->topLevelItemCount(); i++ ) {
            resolvers << ui->scriptList->topLevelItem( i )->data( 1, Qt::DisplayRole ).toString();
        }
        s->setScriptResolvers( resolvers );
        
        if( rescan )
        {
            MusicScanner* scanner = new MusicScanner(s->scannerPath() );
            connect( scanner, SIGNAL( finished() ), scanner, SLOT( deleteLater() ) );
            scanner->start();
        }

        if( rejabber )
        {
            APP->sipHandler()->disconnectPlugins();
            APP->sipHandler()->connectPlugins();
        }
    }
    else
        qDebug() << "Settings dialog cancelled, NOT saving prefs.";

    delete ui;
}


void
SettingsDialog::showPathSelector()
{
    QString path = QFileDialog::getExistingDirectory(
                   this,
                   tr( "Select Music Folder" ),
                   QDesktopServices::storageLocation( QDesktopServices::MusicLocation )
                   );

    if ( path.isEmpty() )
        return;

    ui->lineEditMusicPath->setText( path );
}


void
SettingsDialog::doScan()
{
    // TODO this doesnt really belong here..
    QString path = ui->lineEditMusicPath->text();
    MusicScanner* scanner = new MusicScanner( path );
    connect( scanner, SIGNAL( finished() ), scanner, SLOT( deleteLater() ) );
    scanner->start();

    QMessageBox::information( this, tr( "Scanning Started" ),
                                    tr( "Scanning now, check console output. TODO." ),
                                    QMessageBox::Ok );
}


void
SettingsDialog::onRejected()
{
    m_rejected = true;
}


void
SettingsDialog::changeEvent( QEvent *e )
{
    QDialog::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}


void
SettingsDialog::showProxySettings()
{
    m_proxySettings.exec();
    if ( m_proxySettings.result() == QDialog::Accepted )
        m_proxySettings.saveSettings();
}


void
SettingsDialog::toggleUpnp( bool preferStaticEnabled )
{
    if ( preferStaticEnabled )
        ui->checkBoxUpnp->setEnabled( false );
    else
        ui->checkBoxUpnp->setEnabled( true );
}


void
SettingsDialog::testLastFmLogin()
{
#ifndef NO_LIBLASTFM
    ui->pushButtonTestLastfmLogin->setEnabled( false );
    ui->pushButtonTestLastfmLogin->setText( "Testing..." );

    QString authToken = md5( ( ui->lineEditLastfmUsername->text() + md5( ui->lineEditLastfmPassword->text().toUtf8() ) ).toUtf8() );

    // now authenticate w/ last.fm and get our session key
    QMap<QString, QString> query;
    query[ "method" ] = "auth.getMobileSession";
    query[ "username" ] =  ui->lineEditLastfmUsername->text();
    query[ "authToken" ] = authToken;
    m_testLastFmQuery = lastfm::ws::post( query );

    connect( m_testLastFmQuery, SIGNAL( finished() ), SLOT( onLastFmFinished() ) );
#endif
}


void
SettingsDialog::onLastFmFinished()
{
#ifndef NO_LIBLASTFM
    lastfm::XmlQuery lfm = lastfm::XmlQuery( m_testLastFmQuery->readAll() );

    switch( m_testLastFmQuery->error() )
    {
        case QNetworkReply::NoError:
             qDebug() << "NoError in getting lastfm auth check result";
             if( lfm.children( "error" ).size() > 0 )
             {
                 qDebug() << "ERROR from last.fm:" << lfm.text();
                 ui->pushButtonTestLastfmLogin->setText( tr( "Failed" ) );
                 ui->pushButtonTestLastfmLogin->setEnabled( true );

             } else
             {
                 ui->pushButtonTestLastfmLogin->setText( tr( "Success" ) );
                 ui->pushButtonTestLastfmLogin->setEnabled( false );
             }
             break;
             
        case QNetworkReply::ContentOperationNotPermittedError:
        case QNetworkReply::AuthenticationRequiredError:
            ui->pushButtonTestLastfmLogin->setText( tr( "Failed" ) );
            ui->pushButtonTestLastfmLogin->setEnabled( true );
            break;
            
        default:
            qDebug() << "Couldn't get last.fm auth result";
            ui->pushButtonTestLastfmLogin->setText( tr( "Could not contact server" ) );
            ui->pushButtonTestLastfmLogin->setEnabled( true );
            return;
    }
#endif
}

void
SettingsDialog::authenticateTwitter()
{
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
        TomahawkApp::instance()->sipHandler()->connectPlugins( false, "SIPTWITTER" );
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
SettingsDialog::startPostGotTomahawkStatus()
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
SettingsDialog::postGotTomahawkStatusAuthVerifyReply( const QTweetUser &user )
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
    statUpdate->post( QString( "Got Tomahawk? {" ) + Database::instance()->dbid() + QString( "} (" ) + uuid.mid( 1, 8 ) + QString( ")" ) );
}

void
SettingsDialog::postGotTomahawkStatusUpdateReply( const QTweetStatus& status )
{
    if ( status.id() == 0 )
        QMessageBox::critical( 0, QString("Tweetin' Error"), QString("There was an error posting your status -- sorry!") );
    else
        QMessageBox::information( 0, QString("Tweeted!"), QString("Your tweet has been posted!") );
}
void
SettingsDialog::postGotTomahawkStatusUpdateError( QTweetNetBase::ErrorCode code, const QString& errorMsg )
{
    qDebug() << Q_FUNC_INFO;
    qDebug() << "Error posting Got Tomahawk message, error code is " << code << ", error message is " << errorMsg;
    QMessageBox::critical( 0, QString("Tweetin' Error"), QString("There was an error posting your status -- sorry!") );
}

ProxyDialog::ProxyDialog( QWidget *parent )
    : QDialog( parent )
    , ui( new Ui::ProxyDialog )
{
    ui->setupUi( this );

    // ugly, I know, but...
    QHash<int,int> enumMap;
    int i = 0;
    ui->typeBox->insertItem( i, "No Proxy", QNetworkProxy::NoProxy );
    enumMap[QNetworkProxy::NoProxy] = i++;
    ui->typeBox->insertItem( i, "SOCKS 5", QNetworkProxy::Socks5Proxy );
    enumMap[QNetworkProxy::Socks5Proxy] = i++;

    TomahawkSettings* s = TomahawkSettings::instance();

    ui->typeBox->setCurrentIndex( enumMap[s->proxyType()] );
    ui->hostLineEdit->setText( s->proxyHost() );
    ui->portLineEdit->setText( QString::number( s->proxyPort() ) );
    ui->userLineEdit->setText( s->proxyUsername() );
    ui->passwordLineEdit->setText( s->proxyPassword() );
}


void
ProxyDialog::saveSettings()
{
    qDebug() << Q_FUNC_INFO;

    //First set settings
    TomahawkSettings* s = TomahawkSettings::instance();
    s->setProxyHost( ui->hostLineEdit->text() );

    bool ok;
    qulonglong port = ui->portLineEdit->text().toULongLong( &ok );
    if( ok )
        s->setProxyPort( port );

    s->setProxyUsername( ui->userLineEdit->text() );
    s->setProxyPassword( ui->passwordLineEdit->text() );
    s->setProxyType( ui->typeBox->itemData( ui->typeBox->currentIndex() ).toInt() );

    // Now, if a proxy is defined, set QNAM
    if( s->proxyType() == QNetworkProxy::NoProxy || s->proxyHost().isEmpty() )
        return;

    QNetworkProxy proxy( static_cast<QNetworkProxy::ProxyType>(s->proxyType()), s->proxyHost(), s->proxyPort(), s->proxyUsername(), s->proxyPassword() );
    QNetworkAccessManager* nam = TomahawkUtils::nam();
    nam->setProxy( proxy );
    QNetworkProxy* globalProxy = TomahawkUtils::proxy();
    QNetworkProxy* oldProxy = globalProxy;
    globalProxy = new QNetworkProxy( proxy );
    if( oldProxy )
        delete oldProxy;

    QNetworkProxy::setApplicationProxy( proxy );
}

void 
SettingsDialog::addScriptResolver()
{
    QString resolver = QFileDialog::getOpenFileName( this, tr( "Load script resolver file" ), qApp->applicationDirPath() );
    if( !resolver.isEmpty() ) {
        QFileInfo info( resolver );
        ui->scriptList->addTopLevelItem( new QTreeWidgetItem(  QStringList() << info.baseName() << resolver ) );
        
        TomahawkApp::instance()->addScriptResolver( resolver );
    }
}

void 
SettingsDialog::removeScriptResolver()
{
    // only one selection
    if( !ui->scriptList->selectedItems().isEmpty() ) {
        QString resolver = ui->scriptList->selectedItems().first()->data( 1, Qt::DisplayRole ).toString();
        delete ui->scriptList->takeTopLevelItem( ui->scriptList->indexOfTopLevelItem( ui->scriptList->selectedItems().first() ) );
        
        TomahawkApp::instance()->removeScriptResolver( resolver );
    }
}

void 
SettingsDialog::scriptSelectionChanged()
{
    if( !ui->scriptList->selectedItems().isEmpty() ) {
        ui->removeScript->setEnabled( true );
    } else {
        ui->removeScript->setEnabled( false );
    }
}
