#ifndef LIBTOMAHAWK_UTILS_UUID_H
#define LIBTOMAHAWK_UTILS_UUID_H

#include <QUuid>

// creates 36char ascii guid without {} around it
inline static QString uuid()
{
    // kinda lame, but
    QString q = QUuid::createUuid().toString();
    q.remove( 0, 1 );
    q.chop( 1 );
    return q;
}

#endif // LIBTOMAHAWK_UTILS_UUID_H
