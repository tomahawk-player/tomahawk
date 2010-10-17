#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

#ifndef NO_LIBLASTFM
#include <lastfm/ws.h>
#include <lastfm/XmlQuery>
#endif

#include "tomahawk/tomahawkapp.h"
#include "musicscanner.h"
#include "tomahawksettings.h"
#include <QDesktopServices>


static QString md5( const QByteArray& src )
{
    QByteArray const digest = QCryptographicHash::hash( src, QCryptographicHash::Md5 );
    return QString::fromLatin1( digest.toHex() ).rightJustified( 32, '0' );
}


SettingsDialog::SettingsDialog( QWidget *parent )
    : QDialog( parent )
    , ui( new Ui::SettingsDialog )
    , m_rejected( false )
    , m_testLastFmQuery( 0 )
{
    ui->setupUi( this );
    TomahawkSettings* s = TomahawkApp::instance()->settings();

    ui->checkBoxHttp->setChecked( s->httpEnabled() );
    ui->checkBoxUpnp->setChecked( s->upnpEnabled() );

    // JABBER
    ui->checkBoxJabberAutoConnect->setChecked( s->jabberAutoConnect() );
    ui->jabberUsername->setText( s->jabberUsername() );
    ui->jabberPassword->setText(s->jabberPassword() );
    ui->jabberServer->setText(   s->jabberServer() );
    ui->jabberPort->setValue( s->jabberPort() );

    if ( ui->jabberPort->text().toInt() != 5222 || !ui->jabberServer->text().isEmpty() )
    {
        ui->checkBoxJabberAdvanced->setChecked( true );
    }
    else
    {
        // hide advanved settings
        ui->checkBoxJabberAdvanced->setChecked( false );
        ui->jabberServer->setVisible( false );
        ui->jabberPort->setVisible( false );
        ui->labelJabberServer->setVisible( false );
        ui->labelJabberPort->setVisible( false );
    }

    // MUSIC SCANNER
    ui->lineEditMusicPath->setText( s->scannerPath() );

    // LAST FM
    ui->checkBoxEnableLastfm->setChecked( s->scrobblingEnabled() );
    ui->lineEditLastfmUsername->setText( s->lastFmUsername() );
    ui->lineEditLastfmPassword->setText(s->lastFmPassword() );
    connect( ui->pushButtonTestLastfmLogin, SIGNAL( clicked( bool) ), this, SLOT( testLastFmLogin() ) );
    
    connect( ui->buttonBrowse, SIGNAL( clicked() ),  SLOT( showPathSelector() ) );
    connect( this,             SIGNAL( rejected() ), SLOT( onRejected() ) );
}


SettingsDialog::~SettingsDialog()
{
    qDebug() << Q_FUNC_INFO;

    if ( !m_rejected )
    {
        TomahawkSettings* s = TomahawkApp::instance()->settings();

        // if jabber or scan dir changed, reconnect/rescan
        bool rescan = ui->lineEditMusicPath->text() != s->scannerPath();
        bool rejabber = false;
        if ( ui->jabberUsername->text() != s->jabberUsername() ||
             ui->jabberPassword->text() != s->jabberPassword() ||
             ui->jabberServer->text() != s->jabberServer() ||
             ui->jabberPort->value() != s->jabberPort()
             )
            {
            rejabber = true;
        }

        s->setHttpEnabled(                                  ui->checkBoxHttp->checkState() == Qt::Checked );
        s->setUPnPEnabled(                                  ui->checkBoxUpnp->checkState() == Qt::Checked );

        s->setJabberAutoConnect(                            ui->checkBoxJabberAutoConnect->checkState() == Qt::Checked );
        s->setJabberUsername(                               ui->jabberUsername->text() );
        s->setJabberPassword(                               ui->jabberPassword->text() );
        s->setJabberServer(                                 ui->jabberServer->text() );
        s->setJabberPort(                                   ui->jabberPort->value() );

        s->setScannerPath(                                  ui->lineEditMusicPath->text() );
        
        s->setScrobblingEnabled(                            ui->checkBoxEnableLastfm->isChecked() );
        s->setLastFmUsername(                               ui->lineEditLastfmUsername->text() );
        s->setLastFmPassword(                               ui->lineEditLastfmPassword->text() );

        if( rescan )
        {
            MusicScanner* scanner = new MusicScanner(s->scannerPath() );
            connect( scanner, SIGNAL( finished() ), scanner, SLOT( deleteLater() ) );
            scanner->start();
        }

        if( rejabber )
        {
            APP->reconnectJabber();
        }
        
    }
    else
        qDebug() << "Settings dialog cancelled, NOT saving prefs.";

    delete ui;
}


void SettingsDialog::showPathSelector()
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


void SettingsDialog::doScan()
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


void SettingsDialog::onRejected()
{
    m_rejected = true;
}


void SettingsDialog::changeEvent( QEvent *e )
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


void SettingsDialog::testLastFmLogin()
{
#ifndef NO_LIBLASTFM
    ui->pushButtonTestLastfmLogin->setEnabled( false );
    ui->pushButtonTestLastfmLogin->setText(  "Testing..." );

    QString authToken =  md5( ( ui->lineEditLastfmUsername->text() + md5( ui->lineEditLastfmPassword->text().toUtf8() ) ).toUtf8() );

    // now authenticate w/ last.fm and get our session key
    QMap<QString, QString> query;
    query[ "method" ] = "auth.getMobileSession";
    query[ "username" ] =  ui->lineEditLastfmUsername->text();
    query[ "authToken" ] = authToken;
    m_testLastFmQuery = lastfm::ws::post( query );

    connect( m_testLastFmQuery, SIGNAL( finished() ), SLOT( onLastFmFinished() ) );
#endif
}


void SettingsDialog::onLastFmFinished()
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

