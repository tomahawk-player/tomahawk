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

#ifndef TOMAHAWK_TESTRESULT_H
#define TOMAHAWK_TESTRESULT_H

#include "libtomahawk/Result.h"
#include "libtomahawk/Track.h"
#include "libtomahawk/Source.h"

class TestResult : public QObject
{
    Q_OBJECT

private slots:
    void testGet()
    {
        Tomahawk::result_ptr r;

        r = Tomahawk::Result::get( "", Tomahawk::track_ptr() );
        QVERIFY( !r );

        r = Tomahawk::Result::get( "/tmp/test.mp3", Tomahawk::track_ptr() );
        QVERIFY( !r);

        Tomahawk::track_ptr t = Tomahawk::Track::get( "Artist", "Track" );

        r = Tomahawk::Result::get( "", t );
        QVERIFY( !r);

        r = Tomahawk::Result::get( "/tmp/test.mp3", t );
        QVERIFY( r );
    }
};

#endif
