#ifndef VIEWPAGE_H
#define VIEWPAGE_H

#include <QObject>

#include "typedefs.h"
#include "playlistinterface.h"
#include "utils/tomahawkutils.h"

#include "dllmacro.h"

namespace Tomahawk
{
    
class DLLEXPORT ViewPage
{
public:
    ViewPage() {}

    virtual QWidget* widget() = 0;
    virtual PlaylistInterface* playlistInterface() const = 0;

    virtual QString title() const = 0;
    virtual QString description() const = 0;
    virtual QPixmap pixmap() const { return QPixmap( RESPATH "icons/tomahawk-icon-128x128.png" ); }

    virtual bool showStatsBar() const { return true; }
    virtual bool showModes() const { return false; }
    virtual bool queueVisible() const { return true; }
    
    virtual bool jumpToCurrentTrack() = 0;

private:
};
    
}; // ns

#endif //VIEWPAGE_H
