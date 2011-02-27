#ifndef PLAYLISTINTERFACE_H
#define PLAYLISTINTERFACE_H

#include <QModelIndex>
#include <QWidget>

#include "typedefs.h"

#include "dllmacro.h"
#include "result.h"

class DLLEXPORT PlaylistInterface
{
public:
    enum RepeatMode { NoRepeat, RepeatOne, RepeatAll };
    enum ViewMode { Unknown, Flat, Tree, Album };
    
    PlaylistInterface( QObject* parent = 0 ) : m_object( parent ) {}
    virtual ~PlaylistInterface() {}

    virtual QList< Tomahawk::query_ptr > tracks() = 0;

    virtual int unfilteredTrackCount() const = 0;
    virtual int trackCount() const = 0;

    virtual Tomahawk::result_ptr previousItem() { return siblingItem( -1 ); }
    virtual Tomahawk::result_ptr nextItem() { return siblingItem( 1 ); }
    virtual Tomahawk::result_ptr siblingItem( int itemsAway ) = 0;

    virtual PlaylistInterface::RepeatMode repeatMode() const = 0;
    virtual bool shuffled() const = 0;
    virtual PlaylistInterface::ViewMode viewMode() const { return Unknown; }

    virtual QString filter() const { return m_filter; }
    virtual void setFilter( const QString& pattern ) { m_filter = pattern; }

    QObject* object() const { return m_object; }

public slots:
    virtual void setRepeatMode( RepeatMode mode ) = 0;
    virtual void setShuffled( bool enabled ) = 0;

signals:
    virtual void repeatModeChanged( PlaylistInterface::RepeatMode mode ) = 0;
    virtual void shuffleModeChanged( bool enabled ) = 0;
    virtual void trackCountChanged( unsigned int tracks ) = 0;
    virtual void sourceTrackCountChanged( unsigned int tracks ) = 0;

private:
    QObject* m_object;

    QString m_filter;
};

#endif // PLAYLISTINTERFACE_H
