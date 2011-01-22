#ifndef QUEUEPROXYMODEL_H
#define QUEUEPROXYMODEL_H

#include "playlistproxymodel.h"

#include "dllmacro.h"

class QMetaData;

class DLLEXPORT QueueProxyModel : public PlaylistProxyModel
{
Q_OBJECT

public:
    explicit QueueProxyModel( QObject* parent = 0 );
    ~QueueProxyModel();

    virtual Tomahawk::result_ptr siblingItem( int itemsAway );
};

#endif // QUEUEPROXYMODEL_H
