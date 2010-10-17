#ifndef PLAYLISTINTERFACE_H
#define PLAYLISTINTERFACE_H

#include "playlistitem.h"
#include "tomahawk/collection.h"
#include "tomahawk/source.h"

class PlaylistModelInterface
{
public:
    enum RepeatMode { NoRepeat, RepeatOne, RepeatAll };

    virtual ~PlaylistModelInterface() {}

    virtual PlaylistItem* previousItem() = 0;
    virtual PlaylistItem* nextItem() = 0;
    virtual PlaylistItem* siblingItem( int itemsAway ) = 0;
    virtual void setCurrentItem( const QModelIndex& index ) = 0;

    virtual unsigned int sourceCount() = 0;
    virtual unsigned int collectionCount() = 0;
    virtual unsigned int trackCount() = 0;
    virtual unsigned int artistCount() = 0;

public slots:
    virtual void setRepeatMode( RepeatMode mode ) = 0;
    virtual void setShuffled( bool enabled ) = 0;

signals:
    virtual void repeatModeChanged( PlaylistModelInterface::RepeatMode mode ) = 0;
    virtual void shuffleModeChanged( bool enabled ) = 0;
};

#endif // PLAYLISTINTERFACE_H
