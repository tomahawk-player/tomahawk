#ifndef LIBTOMAHAWK_PLAYLISTREVISION_H
#define LIBTOMAHAWK_PLAYLISTREVISION_H

#include "plentry_ptr.h"

namespace Tomahawk
{

struct PlaylistRevision
{
    QString revisionguid;
    QString oldrevisionguid;
    QList<plentry_ptr> newlist;
    QList<plentry_ptr> added;
    QList<plentry_ptr> removed;
    bool applied; // false if conflict
};

};

#endif // LIBTOMAHAWK_PLAYLISTREVISION_H
