#ifndef DATABASECOMMAND_ALLALBUMS_H
#define DATABASECOMMAND_ALLALBUMS_H

#include <QObject>
#include <QVariantMap>

#include "databasecommand.h"
#include "album.h"
#include "collection.h"
#include "typedefs.h"

#include "dllmacro.h"

class DLLEXPORT DatabaseCommand_AllAlbums : public DatabaseCommand
{
Q_OBJECT
public:
    enum SortOrder {
        None = 0,
        ModificationTime = 1
    };

    explicit DatabaseCommand_AllAlbums( const Tomahawk::collection_ptr& collection, QObject* parent = 0 )
        : DatabaseCommand( parent )
        , m_collection( collection )
        , m_amount( 0 )
        , m_sortOrder( DatabaseCommand_AllAlbums::None )
        , m_sortDescending( false )
    {}

    virtual void exec( DatabaseImpl* );

    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "allalbums"; }

    void setLimit( unsigned int amount ) { m_amount = amount; }
    void setSortOrder( DatabaseCommand_AllAlbums::SortOrder order ) { m_sortOrder = order; }
    void setSortDescending( bool descending ) { m_sortDescending = descending; }

signals:
    void albums( const QList<Tomahawk::album_ptr>&, const Tomahawk::collection_ptr& );
    void done( const Tomahawk::collection_ptr& );

private:
    Tomahawk::collection_ptr m_collection;
    unsigned int m_amount;
    DatabaseCommand_AllAlbums::SortOrder m_sortOrder;
    bool m_sortDescending;
};

#endif // DATABASECOMMAND_ALLALBUMS_H
