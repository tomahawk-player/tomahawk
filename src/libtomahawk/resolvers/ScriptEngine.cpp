/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

#include "ScriptEngine.h"

#include "jobview/ErrorStatusMessage.h"
#include "jobview/JobStatusModel.h"
#include "jobview/JobStatusView.h"
#include "utils/Logger.h"
#include "utils/TomahawkUtils.h"
#include "utils/TomahawkUtilsGui.h"
#include "TomahawkSettings.h"
#include "TomahawkVersion.h"

#include <QDir>
#include <QMessageBox>
#include <QWebFrame>

using namespace Tomahawk;

ScriptEngine::ScriptEngine( JSPlugin* parent )
    : QWebPage( (QObject*) parent )
    , m_parent( parent )
{
    settings()->setAttribute( QWebSettings::OfflineStorageDatabaseEnabled, true );
    settings()->setOfflineStoragePath( TomahawkUtils::appDataDir().path() );
    settings()->setAttribute(QWebSettings::LocalStorageEnabled, true );
    settings()->setLocalStoragePath( TomahawkUtils::appDataDir().path() );
    settings()->setAttribute( QWebSettings::LocalStorageDatabaseEnabled, true );
    settings()->setAttribute( QWebSettings::LocalContentCanAccessFileUrls, true );
    settings()->setAttribute( QWebSettings::LocalContentCanAccessRemoteUrls, true );

    // Tomahawk is not a user agent
    m_header = QWebPage::userAgentForUrl( QUrl() ).replace( QString( "%1/%2" )
               .arg( TOMAHAWK_APPLICATION_NAME )
               .arg( TOMAHAWK_VERSION )
               , "" );
    tLog( LOGVERBOSE ) << "JSResolver Using header" << m_header;

    mainFrame()->setHtml( "<html><body></body></html>", QUrl( "file:///invalid/file/for/security/policy" ) );

    connect( networkAccessManager(), SIGNAL( sslErrors( QNetworkReply*, QList<QSslError> ) ),
                                       SLOT( sslErrorHandler( QNetworkReply*, QList<QSslError> ) ) );
}


void
ScriptEngine::javaScriptConsoleMessage( const QString& message, int lineNumber, const QString& sourceID )
{
    tLog() << "JAVASCRIPT:" << QString( "%1:%2" ).arg( m_scriptPath ).arg( lineNumber ) << message << sourceID;
    #ifdef QT_DEBUG
    QFileInfo scriptPath( m_scriptPath );
    JobStatusView::instance()->model()->addJob( new ErrorStatusMessage( tr( "Resolver Error: %1:%2 %3" ).arg( scriptPath.fileName() ).arg( lineNumber ).arg( message ) ) );
    #endif
}


void
ScriptEngine::sslErrorHandler( QNetworkReply* qnr, const QList<QSslError>& errlist )
{
    tDebug() << Q_FUNC_INFO;

    QByteArray digest = errlist.first().certificate().digest();

    if ( !TomahawkSettings::instance()->isSslCertKnown( digest ) )
    {
        foreach ( const QSslError& err, errlist )
            tDebug() << Q_FUNC_INFO << "SSL error:" << err;

        QMessageBox question( TomahawkUtils::tomahawkWindow() );
        question.setWindowTitle( tr( "SSL Error" ) );
        question.setText( tr( "You have asked Tomahawk to connect securely to <b>%1</b>, but we can't confirm that your connection is secure:<br><br>"
                            "<b>%2</b><br><br>"
                            "Do you want to trust this connection?" )
                            .arg( qnr->url().host() )
                            .arg( errlist.first().errorString() ) );

        question.setStandardButtons( QMessageBox::No );
        question.addButton( tr( "Trust certificate" ), QMessageBox::AcceptRole );

        int result = question.exec();

        //FIXME: discuss whether we want to store rejects, too (needs settings management to remove the decision?)
        if ( result == QMessageBox::AcceptRole )
            TomahawkSettings::instance()->setSslCertTrusted( digest, result == QMessageBox::AcceptRole );
    }

    if ( TomahawkSettings::instance()->isSslCertTrusted( digest ) )
    {
        qnr->ignoreSslErrors();
    }
}


QString
ScriptEngine::userAgentForUrl( const QUrl& url ) const
{
    Q_UNUSED( url );
    return m_header;
}


void
ScriptEngine::setScriptPath( const QString& scriptPath )
{
    m_scriptPath = scriptPath;
}


bool
ScriptEngine::shouldInterruptJavaScript()
{
    return true;
}

