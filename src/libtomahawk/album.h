#ifndef TOMAHAWKALBUM_H
#define TOMAHAWKALBUM_H

#include <QObject>
#include <QSharedPointer>

#include "typedefs.h"
#include "artist.h"
#include "collection.h"
#include "playlistinterface.h"

#include "dllmacro.h"

namespace Tomahawk
{

class DLLEXPORT Album : public QObject, public PlaylistInterface
{
Q_OBJECT

public:
    static album_ptr get( unsigned int id, const QString& name, const Tomahawk::artist_ptr& artist );

    Album( unsigned int id, const QString& name, const Tomahawk::artist_ptr& artist );

    unsigned int id() const { return m_id; }
    QString name() const { return m_name; }
    artist_ptr artist() const { return m_artist; }

    QList<Tomahawk::query_ptr> tracks();

    virtual int trackCount() const { return m_queries.count(); }
    virtual int unfilteredTrackCount() const { return m_queries.count(); }

    virtual Tomahawk::result_ptr siblingItem( int itemsAway );

    virtual PlaylistInterface::RepeatMode repeatMode() const { return PlaylistInterface::NoRepeat; }
    virtual bool shuffled() const { return false; }

    virtual void setRepeatMode( PlaylistInterface::RepeatMode ) {}
    virtual void setShuffled( bool ) {}

    virtual void setFilter( const QString& pattern ) {}

signals:
    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void tracksAdded( const QList<Tomahawk::query_ptr>& tracks );
    void trackCountChanged( unsigned int tracks );
    void sourceTrackCountChanged( unsigned int tracks );

private slots:
    void onTracksAdded( const QList<Tomahawk::query_ptr>& tracks );

private:
    unsigned int m_id;
    QString m_name;

    artist_ptr m_artist;
    QList<Tomahawk::query_ptr> m_queries;

    unsigned int m_currentTrack;
};

}; // ns

#endif
