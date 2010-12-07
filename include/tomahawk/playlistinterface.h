#ifndef PLAYLISTINTERFACE_H
#define PLAYLISTINTERFACE_H

#include <QModelIndex>
#include <QWidget>

#include "tomahawk/typedefs.h"

class PlaylistInterface
{
public:
    enum RepeatMode { NoRepeat, RepeatOne, RepeatAll };

    PlaylistInterface( QObject* parent ) : m_widget( 0 ), m_object( parent ) {}
    virtual ~PlaylistInterface() {}

    virtual int unfilteredTrackCount() const = 0;
    virtual int trackCount() const = 0;

    virtual Tomahawk::result_ptr previousItem() { return siblingItem( -1 ); }
    virtual Tomahawk::result_ptr nextItem() { return siblingItem( 1 ); }
    virtual Tomahawk::result_ptr siblingItem( int itemsAway ) = 0;

    virtual PlaylistInterface::RepeatMode repeatMode() const = 0;
    virtual bool shuffled() const = 0;

    virtual void setFilter( const QString& pattern ) = 0;

    QWidget* widget() const { return m_widget; }
    void setWidget( QWidget* widget ) { m_widget = widget; }

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
    QWidget* m_widget;
    QObject* m_object;
};

#endif // PLAYLISTINTERFACE_H
