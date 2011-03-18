/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "fakecollection.h"
#include "tomahawk/functimeout.h"

FakeCollection::FakeCollection(QObject *parent) :
    Collection("FakeCollection", parent)
{
}

void FakeCollection::load()
{
    QList<QVariantMap> tracks;
    QVariantMap t1, t2, t3;
    t1["artist"] = "0AAAAAArtist 1";
    t1["track"]  = "0TTTTTTrack 1";
    t1["album"]  = "0AAAAAAlbum 1";
    t1["url"]    = "fake://1";
    t1["filesize"] = 5000000;
    t1["duration"] = 300;
    t1["bitrate"]  = 192;
    tracks << t1;

    new Tomahawk::FuncTimeout(5000, boost::bind(&FakeCollection::removeTracks,
                                               this, tracks));

    addTracks(tracks);
    reportFinishedLoading();
 }
