#ifndef QUEUEPROXYMODEL_H
#define QUEUEPROXYMODEL_H

#include "playlistproxymodel.h"

class QMetaData;

class QueueProxyModel : public PlaylistProxyModel
{
Q_OBJECT

public:
    explicit QueueProxyModel( QObject* parent = 0 );
    ~QueueProxyModel();

    virtual Tomahawk::result_ptr siblingItem( int itemsAway );
};

#endif // QUEUEPROXYMODEL_H
