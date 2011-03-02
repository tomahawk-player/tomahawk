#ifndef DATABASECOLLECTION_H
#define DATABASECOLLECTION_H

#include <QDir>

#include "collection.h"
#include "source.h"
#include "typedefs.h"

#include "dllmacro.h"

class DLLEXPORT DatabaseCollection : public Tomahawk::Collection
{
Q_OBJECT

public:
    explicit DatabaseCollection( const Tomahawk::source_ptr& source, QObject* parent = 0 );
    ~DatabaseCollection()
    {
        qDebug() << Q_FUNC_INFO;
    }

    virtual void loadTracks();
    virtual void loadPlaylists();
    virtual void loadDynamicPlaylists();

    virtual QList< Tomahawk::playlist_ptr > playlists();
    virtual QList< Tomahawk::query_ptr > tracks();
    virtual QList< Tomahawk::dynplaylist_ptr > dynamicPlaylists();

public slots:
    virtual void addTracks( const QList<QVariant>& newitems );
    virtual void removeTracks( const QDir& dir );
    
private slots:
    void dynamicPlaylistCreated( const Tomahawk::source_ptr& source, const QVariantList& data );

private:
    bool m_loadedTracks;
};

#endif // DATABASECOLLECTION_H
