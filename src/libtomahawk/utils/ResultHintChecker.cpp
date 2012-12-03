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

ResultHintChecker::ResultHintChecker( const query_ptr& q )
    : QObject( 0 )
    , m_query( q )
    , m_isValid( false )
{
    Q_ASSERT( !m_query.isNull() );
    m_url = q->resultHint();

    connect( m_query.data(), SIGNAL( resolvingFinished( bool ) ), this, SLOT( onResolvingFinished( bool ) ) );
    connect( m_query.data(), SIGNAL( resultsRemoved( const Tomahawk::result_ptr& ) ), this, SLOT( onResultsRemoved( const Tomahawk::result_ptr& ) ) );
    connect( m_query.data(), SIGNAL( resultsChanged() ), this, SLOT( onResultsChanged() ) );
    connect( m_query.data(), SIGNAL( updated() ), this, SLOT( onResultsChanged() ) );
    connect( m_query.data(), SIGNAL( resultsAdded( const QList<Tomahawk::result_ptr>& ) ), this, SLOT( onResultsAdded( const QList<Tomahawk::result_ptr>& ) ) );

    check( m_url );
}


ResultHintChecker::~ResultHintChecker()
{
}

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


void
ResultHintChecker::onResultsRemoved(const result_ptr &result)
{
    qDebug() << Q_FUNC_INFO << result->toString();
}

void
ResultHintChecker::onResultsChanged()
{
    foreach( const result_ptr& rp,  m_query->results() )
    {
        qDebug() << Q_FUNC_INFO << rp->toString();
    }
}


void
ResultHintChecker::checkQueries( const QList< query_ptr >& queries )
{
    foreach ( const query_ptr& query, queries )
        checkQuery( query );
}

void
ResultHintChecker::onResolvingFinished( bool hasResults )
{
    foreach ( const result_ptr& result, m_query->results() )
    {
        qDebug() << Q_FUNC_INFO << "result url():" << result->url();
    }
}

void
ResultHintChecker::onResultsAdded( const QList<Tomahawk::result_ptr>& results )
{
    foreach( const result_ptr& result, results )
    {
        qDebug() << Q_FUNC_INFO << "result url():" << result->url();
    }

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
ResultHintChecker::setResultHint(const QString &url, const bool saveHint)
{

    qDebug() << Q_FUNC_INFO << "Setting new resulthint" << url << "save it? " << saveHint;

    result_ptr foundResult = getResultPtr();

    if ( !foundResult.isNull() )
    {
        qDebug() << Q_FUNC_INFO << "Removing old result " << foundResult->toString();
        m_query->removeResult( foundResult );
    }

    m_query->setResultHint( url );
    m_query->setSaveHTTPResultHint( saveHint );

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
