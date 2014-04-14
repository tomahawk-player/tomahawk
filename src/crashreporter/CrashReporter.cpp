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

#include "CrashReporter.h"

#include <QIcon>
#include <QDebug>
#include <QTimer>
#include <QDir>
#include <QDateTime>

#include "utils/TomahawkUtils.h"

#define LOGFILE TomahawkUtils::appLogDir().filePath( "Tomahawk.log" ).toLocal8Bit()
#define RESPATH ":/data/"
#define PRODUCT_NAME "WaterWolf"

CrashReporter::CrashReporter( const QUrl& url, const QStringList& args )
    : m_reply( 0 )
    , m_url( url )
{
    setWindowIcon( QIcon( RESPATH "icons/tomahawk-icon-128x128.png" ) );

    ui.setupUi( this );
    ui.logoLabel->setPixmap( QPixmap( RESPATH "icons/tomahawk-icon-128x128.png" ).scaled( QSize( 55, 55 ), Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
    ui.progressBar->setRange( 0, 100 );
    ui.progressBar->setValue( 0 );
    ui.progressLabel->setPalette( Qt::gray );

  #ifdef Q_OS_MAC
    QFont f = ui.bottomLabel->font();
    f.setPointSize( 10 );
    ui.bottomLabel->setFont( f );
    f.setPointSize( 11 );
    ui.progressLabel->setFont( f );
    ui.progressLabel->setIndent( 3 );
  #else
    ui.vboxLayout->setSpacing( 16 );
    ui.hboxLayout1->setSpacing( 16 );
    ui.progressBar->setTextVisible( false );
    ui.progressLabel->setIndent( 1 );
    ui.bottomLabel->setDisabled( true );
    ui.bottomLabel->setIndent( 1 );
  #endif //Q_OS_MAC

    m_request = new QNetworkRequest( m_url );

    m_minidump_file_path = args.value( 1 );

    //hide until "send report" has been clicked
    ui.progressBar->setVisible( false );
    ui.button->setVisible( false );
    ui.progressLabel->setVisible( false );
    connect( ui.sendButton, SIGNAL( clicked() ), SLOT( onSendButton() ) );

    adjustSize();
    setFixedSize( size() );
}


CrashReporter::~CrashReporter()
{
    delete m_request;
    delete m_reply;
}


static QByteArray
contents( const QString& path )
{
    QFile f( path );
    f.open( QFile::ReadOnly );
    return f.readAll();
}


void
CrashReporter::send()
{
    QByteArray body;

    // socorro expects a 10 digit build id
    QRegExp rx( "(\\d+\\.\\d+\\.\\d+).(\\d+)" );
    rx.exactMatch( TomahawkUtils::appFriendlyVersion() );
    //QString const version = rx.cap( 1 );
    QString const buildId = rx.cap( 2 ).leftJustified( 10, '0' );

    // add parameters
    typedef QPair<QByteArray, QByteArray> Pair;
    QList<Pair> pairs;
    pairs << Pair( "BuildID", buildId.toUtf8() )
          << Pair( "ProductName",  PRODUCT_NAME)
          << Pair( "Version", TomahawkUtils::appFriendlyVersion().toLocal8Bit() )
          //<< Pair( "Vendor", "Tomahawk" )
          //<< Pair( "timestamp", QByteArray::number( QDateTime::currentDateTime().toTime_t() ) )

//            << Pair("InstallTime", "1357622062")
//            << Pair("Theme", "classic/1.0")
//            << Pair("Version", "30")
//            << Pair("id", "{ec8030f7-c20a-464f-9b0e-13a3a9e97384}")
//            << Pair("Vendor", "Mozilla")
//            << Pair("EMCheckCompatibility", "true")
//            << Pair("Throttleable", "0")
//            << Pair("URL", "http://code.google.com/p/crashme/")
//            << Pair("version", "20.0a1")
//            << Pair("CrashTime", "1357770042")
//            << Pair("ReleaseChannel", "nightly")
//            << Pair("submitted_timestamp", "2013-01-09T22:21:18.646733+00:00")
//            << Pair("buildid", "20130107030932")
//            << Pair("timestamp", "1357770078.646789")
//            << Pair("Notes", "OpenGL: NVIDIA Corporation -- GeForce 8600M GT/PCIe/SSE2 -- 3.3.0 NVIDIA 313.09 -- texture_from_pixmap\r\n")
//            << Pair("StartupTime", "1357769913")
//            << Pair("FramePoisonSize", "4096")
//            << Pair("FramePoisonBase", "7ffffffff0dea000")
//            << Pair("Add-ons", "%7B972ce4c6-7e08-4474-a285-3208198ce6fd%7D:20.0a1,crashme%40ted.mielczarek.org:0.4")
//            << Pair("BuildID", "YYYYMMDDHH")
//            << Pair("SecondsSinceLastCrash", "1831736")
//            << Pair("ProductName", "WaterWolf")
//            << Pair("legacy_processing", "0")
//            << Pair("ProductID", "{ec8030f7-c20a-464f-9b0e-13a3a9e97384}")

            ;



    foreach ( Pair const pair, pairs )
    {
        body += "--thkboundary\r\n";
        body += "Content-Disposition: form-data; name=\"" +
                           pair.first  + "\"\r\n\r\n" +
                           pair.second + "\r\n";
    }

    // TODO: check if dump file actually exists ...

    // add minidump file
    body += "--thkboundary\r\n";
    body += "Content-Disposition: form-data; name=\"upload_file_minidump\"; filename=\""
          + QFileInfo( m_minidump_file_path ).fileName() + "\"\r\n";
    body += "Content-Type: application/octet-stream\r\n";
    body += "\r\n";
    body += contents( m_minidump_file_path );
    body += "\r\n";



    // add logfile
    body += "--thkboundary\r\n";
    body += "Content-Disposition: form-data; name=\"upload_file_tomahawklog\"; filename=\"Tomahawk.log\"\r\n";
    body += "Content-Type: application/x-gzip\r\n";
    body += "\r\n";
    body += qCompress( contents( LOGFILE ) );
    body += "\r\n";
    body += "--thkboundary--\r\n";

    QNetworkAccessManager* nam = new QNetworkAccessManager( this );
    m_request->setHeader( QNetworkRequest::ContentTypeHeader, "multipart/form-data; boundary=thkboundary" );
    m_reply = nam->post( *m_request, body );

#if QT_VERSION < QT_VERSION_CHECK( 5, 0, 0 )
    connect( m_reply, SIGNAL( finished() ), SLOT( onDone() ), Qt::QueuedConnection );
    connect( m_reply, SIGNAL( uploadProgress( qint64, qint64 ) ), SLOT( onProgress( qint64, qint64 ) ) );
#else
    connect( m_reply, &QNetworkReply::finished, this, &CrashReporter::onDone, Qt::QueuedConnection );
    connect( m_reply, &QNetworkReply::uploadProgress, this, &CrashReporter::onProgress );
#endif
}


void
CrashReporter::onProgress( qint64 done, qint64 total )
{
    if ( total )
    {
        QString const msg = tr( "Uploaded %L1 of %L2 KB." ).arg( done / 1024 ).arg( total / 1024 );

        ui.progressBar->setMaximum( total );
        ui.progressBar->setValue( done );
        ui.progressLabel->setText( msg );
    }
}


void
CrashReporter::onDone()
{
    QByteArray data = m_reply->readAll();
    ui.progressBar->setValue( ui.progressBar->maximum() );
    ui.button->setText( tr( "Close" ) );

    QString const response = QString::fromUtf8( data );

    if ( ( m_reply->error() != QNetworkReply::NoError ) || !response.startsWith( "CrashID=" ) )
    {
        onFail( m_reply->error(), m_reply->errorString() );
    }
    else
        ui.progressLabel->setText( tr( "Sent! <b>Many thanks</b>." ) );
}


void
CrashReporter::onFail( int error, const QString& errorString )
{
    ui.button->setText( tr( "Close" ) );
    ui.progressLabel->setText( tr( "Failed to send crash info." ) );
    qDebug() << "Error:" << error << errorString;
}


void
CrashReporter::onSendButton()
{
    ui.progressBar->setVisible( true );
    ui.button->setVisible( true );
    ui.progressLabel->setVisible( true );
    ui.sendButton->setEnabled( false );
    ui.dontSendButton->setEnabled( false );

    adjustSize();
    setFixedSize( size() );

    QTimer::singleShot( 0, this, SLOT( send() ) );
}
