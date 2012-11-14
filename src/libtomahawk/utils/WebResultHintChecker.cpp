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

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>

#include "Query.h"
#include "Result.h"
#include "Source.h"
#include "Pipeline.h"
#include "utils/NetworkReply.h"
#include "utils/Logger.h"

using namespace Tomahawk;


WebResultHintChecker::WebResultHintChecker( const query_ptr& q )
    : QObject( 0 )
    , m_query( q )
{
    Q_ASSERT( !m_query.isNull() );

    m_url = q->resultHint();

    if ( Pipeline::instance()->isResolving( m_query ) )
        connect( m_query.data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( onResolvingFinished( bool ) ) );
    else
        check( QUrl::fromUserInput( m_url ) );
}


WebResultHintChecker::~WebResultHintChecker()
{
}


void
WebResultHintChecker::checkQuery( const query_ptr& query )
{
    if ( !query->resultHint().isEmpty() && query->resultHint().startsWith( "http" ) )
        new WebResultHintChecker( query );
}


void
WebResultHintChecker::checkQueries( const QList< query_ptr >& queries )
{
    foreach ( const query_ptr& query, queries )
        checkQuery( query );
}


void
WebResultHintChecker::onResolvingFinished( bool hasResults )
{
    Q_UNUSED( hasResults );

    check( QUrl::fromUserInput( m_url ) );
}


void
WebResultHintChecker::check( const QUrl &url )
{
    // Nothing to do
    if ( url.isEmpty() || !url.toString().startsWith( "http" ) )
    {
        if ( !url.isEmpty() || m_query->saveHTTPResultHint() )
            removeHint();

        deleteLater();
        return;
    }

    NetworkReply* reply = new NetworkReply( TomahawkUtils::nam()->head( QNetworkRequest( url ) ) );
    connect( reply, SIGNAL( finished() ), SLOT( headFinished() ) );
}


void
WebResultHintChecker::removeHint()
{
    tLog() << "Removing HTTP result from query since HEAD request failed to verify it was a valid url:" << m_url;

    result_ptr foundResult;
    foreach ( const result_ptr& result, m_query->results() )
    {
        if ( result->url() == m_url )
        {
            foundResult = result;
            break;
        }
    }

    if ( !foundResult.isNull() )
        m_query->removeResult( foundResult );
    if ( m_query->resultHint() == m_url )
        m_query->setResultHint( QString() );
    m_query->setSaveHTTPResultHint( false );
}


void
WebResultHintChecker::headFinished()
{
    NetworkReply* r = qobject_cast<NetworkReply*>( sender() );
    r->deleteLater();

    if ( r->reply()->error() != QNetworkReply::NoError )
    {
        // Error getting headers for the http resulthint, remove it from the result
        // as it's definitely not playable
        removeHint();
    }

    deleteLater();
}
