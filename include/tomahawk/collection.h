/*
    The collection  - acts as container for someones music library
    load() -> async populate by calling addArtists etc,
    then finishedLoading() is emitted.
    then use artists() etc to get the data.
*/

#ifndef TOMAHAWK_COLLECTION_H
#define TOMAHAWK_COLLECTION_H

#include <QHash>
#include <QList>
#include <QSharedPointer>
#include <QDebug>

#include "tomahawk/functimeout.h"
#include "tomahawk/playlist.h"
#include "tomahawk/source.h"
#include "tomahawk/typedefs.h"

namespace Tomahawk
{

/*
    Call load(), then wait for the finishedLoading() signal,
    then call tracks() to get all tracks.
 */

class Collection : public QObject
{
Q_OBJECT

public:
    Collection( const source_ptr& source, const QString& name, QObject* parent = 0 );
    virtual ~Collection();

    void invokeSlotTracks( QObject* obj, const char* slotname, const QList<QVariant>& val, collection_ptr collection );

    virtual QString name() const;

    virtual void loadPlaylists() = 0;

    virtual Tomahawk::playlist_ptr playlist( const QString& guid );
    virtual void addPlaylist( const Tomahawk::playlist_ptr& p );
    virtual void deletePlaylist( const Tomahawk::playlist_ptr& p );

    /// async calls that fetch data from DB/whatever:
    void loadTracks( QObject* obj, const char* slotname );

    virtual const QList< Tomahawk::playlist_ptr >& playlists() const { return m_playlists; }

    bool isLoaded() const { return m_loaded; }

    const source_ptr& source() const { return m_source; }
    unsigned int lastmodified() const { return m_lastmodified; }

    static bool trackSorter( const QVariant& left, const QVariant &right );

signals:
    void tracksAdded( const QList<QVariant>&, Tomahawk::collection_ptr );
    void tracksRemoved( const QList<QVariant>&, Tomahawk::collection_ptr );

    void playlistsAdded( const QList<Tomahawk::playlist_ptr>& );
    void playlistsDeleted( const QList<Tomahawk::playlist_ptr>& );

public slots:
    virtual void addTracks( const QList<QVariant> &newitems ) = 0;
    virtual void removeTracks( const QList<QVariant> &olditems ) = 0;

    void setPlaylists( const QList<Tomahawk::playlist_ptr>& plists )
    {
        qDebug() << Q_FUNC_INFO << plists.length();
        m_playlists.append( plists );
        if( !m_loaded )
        {
            m_loaded = true;
            emit playlistsAdded( plists );
        }
    }

protected:
    virtual void loadAllTracks( boost::function<void( const QList<QVariant>&, collection_ptr )> callback ) = 0;

    QString m_name;
    bool m_loaded;
    unsigned int m_lastmodified; // unix time of last change to collection

private:
    source_ptr m_source;
    QList< Tomahawk::playlist_ptr > m_playlists;
};

}; // ns

inline uint qHash( const QSharedPointer<Tomahawk::Collection>& key )
{
    return qHash( (void *)key.data() );
}

#endif // TOMAHAWK_COLLECTION_H
