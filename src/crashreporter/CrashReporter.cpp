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
#include <QHttp>

#include "utils/TomahawkUtils.h"

#define LOGFILE TomahawkUtils::appLogDir().filePath( "Tomahawk.log" ).toLocal8Bit()
#define RESPATH ":/data/"


CrashReporter::CrashReporter( const QStringList& args )
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

    m_http = new QHttp( "oops.tomahawk-player.org", 80, this );

    connect( m_http, SIGNAL( done( bool ) ), SLOT( onDone() ), Qt::QueuedConnection );
    connect( m_http, SIGNAL( dataSendProgress( int, int ) ), SLOT( onProgress( int, int ) ) );

    m_dir = args.value( 1 );
    m_minidump = m_dir + '/' + args.value( 2 ) + ".dmp";
    m_product_name = args.value( 3 );

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
    delete m_http;
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
    QString const version = rx.cap( 1 );
    QString const buildId = rx.cap( 2 ).leftJustified( 10, '0' );

    // add parameters
    typedef QPair<QByteArray, QByteArray> Pair;
    QList<Pair> pairs;
    pairs << Pair( "BuildID", buildId.toUtf8() )
          << Pair( "ProductName", m_product_name.toUtf8() )
          << Pair( "Version", TomahawkUtils::appFriendlyVersion().toLocal8Bit() )
          << Pair( "Vendor", "Tomahawk" )
          << Pair( "timestamp", QByteArray::number( QDateTime::currentDateTime().toTime_t() ) );

    foreach ( Pair const pair, pairs )
    {
        body += "--thkboundary\r\n";
        body += "Content-Disposition: form-data; name=\"" +
                           pair.first  + "\"\r\n\r\n" +
                           pair.second + "\r\n";
    }

    // add minidump file
    body += "--thkboundary\r\n";
    body += "Content-Disposition: form-data; name=\"upload_file_minidump\"; filename=\""
          + QFileInfo( m_minidump ).fileName() + "\"\r\n";
    body += "Content-Type: application/octet-stream\r\n";
    body += "\r\n";
    body += contents( m_minidump );
    body += "\r\n";

    // add logfile
    body += "--thkboundary\r\n";
    body += "Content-Disposition: form-data; name=\"upload_file_tomahawklog\"; filename=\"Tomahawk.log\"\r\n";
    body += "Content-Type: application/x-gzip\r\n";
    body += "\r\n";
    body += qCompress( contents( LOGFILE ) );
    body += "\r\n";
    body += "--thkboundary--\r\n";

    QHttpRequestHeader header( "POST", "/addreport.php" );
    header.setContentType( "multipart/form-data; boundary=thkboundary" );
    header.setValue( "HOST", "oops.tomahawk-player.org" );

    m_http->request( header, body );
}


void
CrashReporter::onProgress( int done, int total )
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
    QByteArray data = m_http->readAll();
    ui.progressBar->setValue( ui.progressBar->maximum() );
    ui.button->setText( tr( "Close" ) );

    QString const response = QString::fromUtf8( data );

    if ( m_http->error() != QHttp::NoError || !response.startsWith( "CrashID=" ) )
    {
        onFail( m_http->error(), m_http->errorString() );
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
