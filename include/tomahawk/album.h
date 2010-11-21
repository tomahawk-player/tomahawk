#ifndef TOMAHAWKALBUM_H
#define TOMAHAWKALBUM_H

#include <QObject>
#include <QSharedPointer>

#include "tomahawk/typedefs.h"
#include "tomahawk/artist.h"
#include "tomahawk/collection.h"

#include "tomahawk/playlistinterface.h"

namespace Tomahawk
{

class Album : public QObject, public PlaylistInterface
{
Q_OBJECT

public:
    static album_ptr get( unsigned int id, const QString& name, const Tomahawk::artist_ptr& artist, const Tomahawk::collection_ptr& collection );

    Album( unsigned int id, const QString& name, const Tomahawk::artist_ptr& artist, const Tomahawk::collection_ptr& collection );

    unsigned int id() const { return m_id; }
    QString name() const { return m_name; }
    artist_ptr artist() const { return m_artist; }

    Tomahawk::collection_ptr collection() const { return m_collection; }
    QList<Tomahawk::query_ptr> tracks();

    virtual int trackCount() const { return m_queries.count(); }
    virtual Tomahawk::result_ptr previousItem() { return siblingItem( -1 ); }
    virtual Tomahawk::result_ptr nextItem() { return siblingItem( 1 ); }
    virtual Tomahawk::result_ptr siblingItem( int itemsAway );

    virtual PlaylistInterface::RepeatMode repeatMode() const { return PlaylistInterface::NoRepeat; }
    virtual bool shuffled() const { return false; }

    virtual void setRepeatMode( PlaylistInterface::RepeatMode ) {}
    virtual void setShuffled( bool ) {}

signals:
    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void tracksAdded( const QList<Tomahawk::query_ptr>& tracks, const Tomahawk::collection_ptr& );
    void trackCountChanged( unsigned int tracks );

private slots:
    void onTracksAdded( const QList<Tomahawk::query_ptr>& tracks, const Tomahawk::collection_ptr& collection );

private:
    unsigned int m_id;
    QString m_name;

    artist_ptr m_artist;
    QList<Tomahawk::query_ptr> m_queries;
    Tomahawk::collection_ptr m_collection;

    unsigned int m_currentTrack;
};

}; // ns

#endif
