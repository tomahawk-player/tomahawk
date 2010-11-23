#ifndef PLAYLISTINTERFACE_H
#define PLAYLISTINTERFACE_H

#include <QModelIndex>

#include "tomahawk/typedefs.h"

class PlaylistInterface
{
public:
    enum RepeatMode { NoRepeat, RepeatOne, RepeatAll };

    PlaylistInterface() {}
    virtual ~PlaylistInterface() {}

    virtual int trackCount() const = 0;

    virtual Tomahawk::result_ptr previousItem() { return siblingItem( -1 ); }
    virtual Tomahawk::result_ptr nextItem() { return siblingItem( 1 ); }
    virtual Tomahawk::result_ptr siblingItem( int itemsAway ) = 0;

    virtual PlaylistInterface::RepeatMode repeatMode() const = 0;
    virtual bool shuffled() const = 0;

public slots:
    virtual void setRepeatMode( RepeatMode mode ) = 0;
    virtual void setShuffled( bool enabled ) = 0;

signals:
    virtual void repeatModeChanged( PlaylistInterface::RepeatMode mode ) = 0;
    virtual void shuffleModeChanged( bool enabled ) = 0;
    virtual void trackCountChanged( unsigned int tracks ) = 0;
};

#endif // PLAYLISTINTERFACE_H
