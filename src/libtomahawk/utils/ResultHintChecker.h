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
#ifndef RESULT_HINT_CHECKER_H
#define RESULT_HINT_CHECKER_H

#include "Typedefs.h"
#include <QObject>

namespace Tomahawk
{

class ResultHintChecker : public QObject
{
    Q_OBJECT
public:
    explicit ResultHintChecker( const query_ptr& q, qint64 expires = 0 );
    virtual ~ResultHintChecker();

    static void checkQuery( const query_ptr& query );
    static void checkQueries( const QList< query_ptr >& queries );

    bool isValid();

    QString url() const { return m_url; }
    QString resultHint() const;

    void setResultUrl( const QString &url );
    void setUrl( const QString& url ) { m_url = url; }
    void setExpires( qint64 expires );
    void removeHint();

private:
    void check( const QUrl& url );

    result_ptr getResultPtr();
    query_ptr m_query;
    QString m_url;
    qint64 m_expires;
    bool m_isValid;
};

}

#endif
