#ifndef DATABASECOMMAND_ALLTRACKS_H
#define DATABASECOMMAND_ALLTRACKS_H

#include <QObject>
#include <QVariantMap>

#include "databasecommand.h"
#include "tomahawk/album.h"
#include "tomahawk/collection.h"
#include "tomahawk/typedefs.h"

class DatabaseCommand_AllTracks : public DatabaseCommand
{
Q_OBJECT
public:
    enum SortOrder {
        None = 0,
        ModificationTime = 1,
        AlbumPosition = 2
    };

    explicit DatabaseCommand_AllTracks( const Tomahawk::collection_ptr& collection, QObject* parent = 0 )
        : DatabaseCommand( parent )
        , m_collection( collection )
        , m_album( 0 )
        , m_amount( 0 )
        , m_sortOrder( DatabaseCommand_AllTracks::None )
        , m_sortDescending( false )
    {}

    virtual void exec( DatabaseImpl* );

    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "alltracks"; }

    void setAlbum( Tomahawk::Album* album ) { m_album = album; }
    void setLimit( unsigned int amount ) { m_amount = amount; }
    void setSortOrder( DatabaseCommand_AllTracks::SortOrder order ) { m_sortOrder = order; }
    void setSortDescending( bool descending ) { m_sortDescending = descending; }

signals:
    void tracks( const QList<Tomahawk::query_ptr>&, const Tomahawk::collection_ptr& );
    void done( const Tomahawk::collection_ptr& );

private:
    Tomahawk::collection_ptr m_collection;
    Tomahawk::Album* m_album;
    unsigned int m_amount;
    DatabaseCommand_AllTracks::SortOrder m_sortOrder;
    bool m_sortDescending;
};

#endif // DATABASECOMMAND_ALLTRACKS_H
