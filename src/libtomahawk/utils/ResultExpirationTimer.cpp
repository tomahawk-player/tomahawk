/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
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
#include "CustomResultHintChecker.h"
#include "Query.h"
#include "Result.h"
#include "utils/Logger.h"

using namespace Tomahawk;

ResultExpirationTimer* ResultExpirationTimer::s_instance = 0;

ResultExpirationTimer*
ResultExpirationTimer::instance()
{
    if ( !s_instance )
        s_instance = new ResultExpirationTimer;

    return s_instance;
}


/**
 * @brief ResultExpirationTimer::ResultExpirationTimer
 */
ResultExpirationTimer::ResultExpirationTimer( QObject *parent )
    :QObject(parent)
    ,m_currentTimeout( 0 )
{
    connect( this, SIGNAL( resultAdded() ), this, SLOT( updateTimer() ) );
}


/**
 * @brief ResultExpirationTimer::updateTimer
 * Takes first pair and set a new expire timer
 */
void
ResultExpirationTimer::updateTimer()
{
    QMap< qint64, QList< result_ptr > >::Iterator it = m_results.begin();

    if ( it != m_results.end() && ( it.key() < m_currentTimeout || m_currentTimeout == 0 ) )
    {
        m_currentTimeout = it.key();
        qint64 expire = expires( m_currentTimeout )*1000;

        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "SET TIMER TO EXPIRE IN " << m_currentTimeout << expire << "msec";
        QTimer::singleShot( expire, this, SLOT( onExpired() ) );
    }
}


/**
 * @brief ResultExpirationTimer::expires
 * @param expires
 * @return qint64
 */
qint64
ResultExpirationTimer::expires( qint64 expires ) const
{
    qint64 currentEpoch = QDateTime::currentMSecsSinceEpoch()/1000;
    qint64 expiresInSeconds = expires-currentEpoch;

    if ( expiresInSeconds > 0 )
    {
        return expiresInSeconds;
    }
    return 0;
}


/**
 * @brief ResultExpirationTimer::onExpired
 * onExpired slot, takes first pair, and force a new
 * url lookup on it
 */
void
ResultExpirationTimer::onExpired()
{
    qDebug() << Q_FUNC_INFO;
    QMap< qint64, QList< result_ptr > >::Iterator it = m_results.begin();

    if ( it != m_results.end() )
    {
        foreach ( const result_ptr& result, it.value() )
        {
            /// @note: Must be a Custom if it has expiration
            new CustomResultHintChecker( m_queries[ result->url() ], result->url() );
        }
        m_results.remove( it.key() );
        m_currentTimeout = 0;
    }

}

/**
 * @brief ResultExpirationTimer::addResult
 * @param query_ptr
 * @param result_ptr
 * Adds a result to be checked on expiration
 */
void
ResultExpirationTimer::addResult( const query_ptr& query, const Tomahawk::result_ptr& result )
{
    if ( result->getExpires() >= 0 )
    {
        m_results[ result->getExpires() ].append( result );
        m_queries[ result->url() ] = query;
        emit resultAdded();
    }
}
