/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Leo Franchi <lfranchi@kde.org>
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
#include "WebResultHintChecker.h"

#include "Query.h"
#include "Result.h"
#include "Source.h"
#include "utils/Closure.h"
#include "utils/Logger.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

using namespace Tomahawk;

WebResultHintChecker::WebResultHintChecker( const query_ptr& q )
    : QObject( 0 )
    , m_query( q )
{
    m_url = q->resultHint();

    foreach ( const result_ptr& result, q->results() )
    {
        if ( result->url() == m_url )
        {
            m_result = result;
            break;
        }
    }

    // Nothing to do
    if ( m_url.isEmpty() || !m_url.startsWith( "http" ) )
    {
        deleteLater();
        return;
    }

    check( m_url );
}

WebResultHintChecker::~WebResultHintChecker()
{

}


void
WebResultHintChecker::check( const QString &url )
{
    QNetworkReply* reply = TomahawkUtils::nam()->head( QNetworkRequest( QUrl( url ) ) );
    NewClosure( reply, SIGNAL( finished() ), this, SLOT( headFinished( QNetworkReply* ) ), reply );
}


void
WebResultHintChecker::headFinished( QNetworkReply* reply )
{
    reply->deleteLater();

    const QUrl redir = reply->attribute( QNetworkRequest::RedirectionTargetAttribute ).toUrl();
    if ( redir.isValid() )
    {
        const QUrl url = reply->url().resolved( redir );
        check( url.toString() );

        return;
    }
    else if ( reply->error() != QNetworkReply::NoError )
    {
        // Error getting headers for the http resulthint, remove it from the result
        // as it's definitely not playable
        tLog() << "Removing HTTP result from query since HEAD request failed to verify it was a valid url:" << m_url;
        if ( !m_result.isNull() )
            m_query->removeResult( m_result );
        if ( m_query->resultHint() == m_url )
            m_query->setResultHint( QString() );
        m_query->setSaveHTTPResultHint( false );
    }

    deleteLater();
}
