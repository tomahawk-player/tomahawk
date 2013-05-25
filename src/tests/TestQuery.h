/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
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

#ifndef TOMAHAWK_TESTQUERY_H
#define TOMAHAWK_TESTQUERY_H

#include "libtomahawk/Query.h"
#include "libtomahawk/Source.h"

class TestQuery : public QObject
{
    Q_OBJECT

private slots:
    void testGet()
    {
        Tomahawk::query_ptr q = Tomahawk::Query::get( "", "", "" );
        QVERIFY( !q );
    }
};

#endif
