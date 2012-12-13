/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2012, Hugo Lindström <hugolm84@gmail.com>
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

#include "ResultExpirationTimer.h"
#include "WebResultHintChecker.h"
#include "CustomResultHintChecker.h"
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

/**
 * @brief ResultHintChecker::checkQuery
 * @param query
 * Static function to initiate resulthint checkers
 */
void
ResultHintChecker::checkQuery( const query_ptr& query )
{
    if ( !query->resultHint().isEmpty() && TomahawkUtils::whitelistedHttpResultHint( query->resultHint() ) )
        new WebResultHintChecker( query );

    else if ( !query->resultHint().isEmpty() && TomahawkUtils::whitelistedCustomProtocolResultHint( query->resultHint() ) )
        new CustomResultHintChecker( query );
}

ResultHintChecker::ResultHintChecker( const query_ptr& q, qint64 expires )
    : QObject( 0 )
    , m_query( q )
    , m_isValid( false )
    , m_expires( expires )
{
    Q_ASSERT( !m_query.isNull() );
    m_url = q->resultHint();
    check( m_url );
}


ResultHintChecker::~ResultHintChecker()
{
}

void
ResultHintChecker::checkQueries( const QList< query_ptr >& queries )
{
    foreach ( const query_ptr& query, queries )
        checkQuery( query );
}

void
ResultHintChecker::setExpires( qint64 expires )
{
    m_expires = expires;
}

QString
ResultHintChecker::resultHint() const
{
    return m_query->resultHint();
}

bool
ResultHintChecker::isValid()
{
    return m_isValid;
}


void
ResultHintChecker::check( const QUrl &url )
{

    qDebug() << Q_FUNC_INFO << url;
    // Nothing to do
    if ( url.isEmpty() ||
         ( !TomahawkUtils::whitelistedHttpResultHint( url.toString() ) &&
           !TomahawkUtils::whitelistedCustomProtocolResultHint( url.toString() )
         ) )
    {
        if ( !url.isEmpty() || m_query->saveHTTPResultHint() )
            removeHint();

        deleteLater();
        return;
    }

    m_isValid = true;

}

result_ptr
ResultHintChecker::getResultPtr()
{
    result_ptr foundResult;
    foreach ( const result_ptr& result, m_query->results() )
    {
        if ( result->url() == m_url )
        {
            foundResult = result;
            break;
        }
    }

    return foundResult;
}

void
ResultHintChecker::setResultUrl( const QString &url )
{

    result_ptr result = getResultPtr();

    if ( !result.isNull() )
    {
        result->setUrl( url );
        if( m_expires <= 0 )
            return;

        result->setExpires( m_expires );
        ResultExpirationTimer::instance()->addResult( m_query, result );
        return;
    }

    qDebug() << Q_FUNC_INFO << "Error updating result" << url << m_expires;
}


void
ResultHintChecker::removeHint()
{
    tLog() << "Removing invalid resulthint from query:" << m_url;

    result_ptr foundResult = getResultPtr();

    if ( !foundResult.isNull() )
           m_query->removeResult( foundResult );

    if ( m_query->resultHint() == m_url )
    {
        m_query->setResultHint( QString() );
    }

    m_query->setSaveHTTPResultHint( false );
}
