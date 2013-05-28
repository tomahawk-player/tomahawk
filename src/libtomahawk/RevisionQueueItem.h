#ifndef LIBTOMAHAWK_REVISIONQUEUEITEM_H
#define LIBTOMAHAWK_REVISIONQUEUEITEM_H

#include "plentry_ptr.h"

namespace Tomahawk
{

struct RevisionQueueItem
{
public:
    QString newRev;
    QString oldRev;
    QList< plentry_ptr > entries;
    bool applyToTip;

    RevisionQueueItem( const QString& nRev, const QString& oRev, const QList< plentry_ptr >& e, bool latest ) :
        newRev( nRev ), oldRev( oRev), entries( e ), applyToTip( latest ) {}
};

};

#endif // LIBTOMAHAWK_REVISIONQUEUEITEM_H
