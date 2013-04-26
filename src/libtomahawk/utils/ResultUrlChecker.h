/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef WEB_RESULT_HINT_CHECKER_H
#define WEB_RESULT_HINT_CHECKER_H

#include "Typedefs.h"
#include "utils/NetworkReply.h"

#include <QObject>
#include <QSet>

namespace Tomahawk
{

class ResultUrlChecker : public QObject
{
    Q_OBJECT
public:
    ResultUrlChecker( const query_ptr& query, const QList< result_ptr >& results );
    virtual ~ResultUrlChecker();

    query_ptr query() const { return m_query; }
    QList< result_ptr > results() const { return m_results; }
    QList< result_ptr > validResults() const { return m_validResults; }

signals:
    void done();

private slots:
    void check();
    void headFinished();

private:
    query_ptr m_query;
    QList< result_ptr > m_results;
    QList< result_ptr > m_validResults;
    QHash< NetworkReply*, Tomahawk::result_ptr > m_replies;
};

}

#endif
