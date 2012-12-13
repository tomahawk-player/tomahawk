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
#ifndef RESULT_EXPIRATION_TIMER_H
#define RESULT_EXPIRATION_TIMER_H

#include "Typedefs.h"
#include <QObject>
#include <QTimer>
#include <QDateTime>
#include "Resolver.h"
namespace Tomahawk
{

/**
 * @brief The ResultExpirationTimer class
 * Use this when you have a result that expires in a certain time
 *
 *  (result_ptr) result->setExpires( expireTimestamp );
 *  ResultExpirationTimer::instance()->addResult( query_ptr, result_ptr );
 *  Will fire on next expiration ( timestamp - msecSinceEpoch ) and trigger
 *  result url update
 */
class ResultExpirationTimer : public QObject
{
    Q_OBJECT
public:
    static ResultExpirationTimer* instance();

    void addResult( const query_ptr& query, const Tomahawk::result_ptr& result );
    void addResult(const Tomahawk::result_ptr& result);
signals:
    void resultAdded();

private slots:
    void updateTimer();
    void onExpired();

private:
    ResultExpirationTimer( QObject *parent = 0 );
    static ResultExpirationTimer* s_instance;

    QMap< qint64, QList<Tomahawk::result_ptr > > m_results;
    QMap< QString, query_ptr > m_queries;

    qint64 expires( qint64 expires ) const;
    qint64 m_currentTimeout;

};

}

#endif
