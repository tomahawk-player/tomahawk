#ifndef TOMAHAWKARTIST_H
#define TOMAHAWKARTIST_H

#include <QObject>
#include <QSharedPointer>

#include "typedefs.h"

#include "playlistinterface.h"

#include "dllmacro.h"

namespace Tomahawk
{

class DLLEXPORT Artist : public QObject, public PlaylistInterface
{
Q_OBJECT

public:
    static artist_ptr get( unsigned int id, const QString& name, const Tomahawk::collection_ptr& collection );
    Artist( unsigned int id, const QString& name, const Tomahawk::collection_ptr& collection );

    Artist();
    virtual ~Artist();
    
    unsigned int id() const { return m_id; }
    QString name() const { return m_name; }

	Tomahawk::collection_ptr collection() const;

    QList<Tomahawk::query_ptr> tracks();

    virtual int trackCount() const { return 0; }
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

    void tracksAdded( const QList<Tomahawk::query_ptr>& tracks, const Tomahawk::collection_ptr& );
    void trackCountChanged( unsigned int tracks );
    void sourceTrackCountChanged( unsigned int tracks );

private slots:
    void onTracksAdded( const QList<Tomahawk::query_ptr>& tracks, const Tomahawk::collection_ptr& collection );

private:
    unsigned int m_id;
    QString m_name;

    QList<Tomahawk::query_ptr> m_queries;
    unsigned int m_currentTrack;
    Tomahawk::collection_ptr m_collection;
};

}; // ns

#endif
