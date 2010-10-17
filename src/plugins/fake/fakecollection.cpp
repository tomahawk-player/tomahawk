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
