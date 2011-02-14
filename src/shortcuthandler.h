#ifndef SHORTCUTHANDLER_H
#define SHORTCUTHANDLER_H

#include <QObject>

namespace Tomahawk {
/**
  Base class for various shortcut plugins on different platforms
  */
class ShortcutHandler : public QObject
{
    Q_OBJECT
public:
    virtual ~ShortcutHandler();

signals:
    // add more as needed
    void playPause();
    void pause();
    void stop();
    void previous();
    void next();

    void volumeUp();
    void volumeDown();
    void mute();
protected:
    explicit ShortcutHandler( QObject *parent = 0 );

};

}

#endif // SHORTCUTHANDLER_H
