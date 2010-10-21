#ifndef PLAYLISTINTERFACE_H
#define PLAYLISTINTERFACE_H

#include <QModelIndex>

#include "plitem.h"
#include "tomahawk/collection.h"
#include "tomahawk/source.h"

class PlaylistInterface
{
public:
    enum RepeatMode { NoRepeat, RepeatOne, RepeatAll };

    PlaylistInterface()
    {
        m_rootItem = new PlItem();
    }

    virtual ~PlaylistInterface() {}

    virtual void setCurrentItem( const QModelIndex& ) = 0;

    virtual int trackCount() const = 0;

    virtual PlItem* previousItem() = 0;
    virtual PlItem* nextItem() = 0;
    virtual PlItem* siblingItem( int itemsAway ) = 0;

    virtual PlaylistInterface::RepeatMode repeatMode() const = 0;
    virtual bool shuffled() const = 0;

    PlItem* itemFromIndex( const QModelIndex& index ) const
    {
        if ( index.isValid() )
            return static_cast<PlItem*>( index.internalPointer() );
        else
        {
            return m_rootItem;
        }
    }

public slots:
    virtual void setRepeatMode( RepeatMode mode ) = 0;
    virtual void setShuffled( bool enabled ) = 0;

signals:
    virtual void repeatModeChanged( PlaylistInterface::RepeatMode mode ) = 0;
    virtual void shuffleModeChanged( bool enabled ) = 0;
    virtual void trackCountChanged( unsigned int tracks ) = 0;

public:
    PlItem* m_rootItem;
    bool m_flat;
};

#endif // PLAYLISTINTERFACE_H
