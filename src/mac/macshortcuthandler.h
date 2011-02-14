#ifndef MACSHORTCUTHANDLER_H
#define MACSHORTCUTHANDLER_H

#include "shortcuthandler.h"

#include <QObject>

namespace Tomahawk {


class MacShortcutHandler : public ShortcutHandler
{
    Q_OBJECT
public:
    explicit MacShortcutHandler(QObject *parent = 0);

    void macMediaKeyPressed( int key );
};

}

#endif // MACSHORTCUTHANDLER_H
