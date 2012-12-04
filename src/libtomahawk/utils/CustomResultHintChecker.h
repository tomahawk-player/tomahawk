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
#ifndef Custom_RESULT_HINT_CHECKER_H
#define Custom_RESULT_HINT_CHECKER_H

#include "Typedefs.h"
#include <QUrl>
#include <QObject>
#include "ResultHintChecker.h"

namespace Tomahawk
{

/**
 * @brief The CustomResultHintChecker class
 * This class is to be used with custom protocol to retrieve
 * streamable urls for resulthints.
 */
class CustomResultHintChecker : public ResultHintChecker
{
    Q_OBJECT
public:
    CustomResultHintChecker( const query_ptr& q );
    virtual ~CustomResultHintChecker(){}
private slots:
    /* HotNewHiphop specific */
    void hnhhFinished();
    void handleHnhhResult( result_ptr result );
private:
    void handleResultHint();
    QString m_streamUrl;
    query_ptr m_query;
};


}

#endif
