#include "macshortcuthandler.h"

#include <QDebug>
#include <IOKit/hidsystem/ev_keymap.h>

using namespace Tomahawk;

MacShortcutHandler::MacShortcutHandler(QObject *parent) :
    Tomahawk::ShortcutHandler(parent)
{

}

void
MacShortcutHandler::macMediaKeyPressed( int key )
{
    switch (key) {
      case NX_KEYTYPE_PLAY:
        qDebug() << "emitting PlayPause pressed";
        emit playPause();
        break;
      case NX_KEYTYPE_FAST:
        qDebug() << "emitting next pressed";
        emit next();
        break;
      case NX_KEYTYPE_REWIND:
        qDebug() << "emitting prev pressed";
        emit previous();
        break;
    }
}
