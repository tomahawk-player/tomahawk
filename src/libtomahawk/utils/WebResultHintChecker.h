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
#ifndef WEB_RESULT_HINT_CHECKER_H
#define WEB_RESULT_HINT_CHECKER_H

#include "Typedefs.h"

#include <QObject>

class QNetworkReply;

namespace Tomahawk {

class WebResultHintChecker : public QObject
{
    Q_OBJECT
public:
    WebResultHintChecker( const query_ptr& q );
    virtual ~WebResultHintChecker();

private slots:
    void headFinished( QNetworkReply* reply );

private:
    void check( const QString& url );

    query_ptr m_query;
    result_ptr m_result;
    QString m_url;
};

}

#endif
