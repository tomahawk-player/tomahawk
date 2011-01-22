#ifndef COLLECTIONPROXYMODEL_H
#define COLLECTIONPROXYMODEL_H

#include "trackproxymodel.h"

#include "dllmacro.h"

class DLLEXPORT CollectionProxyModel : public TrackProxyModel
{
Q_OBJECT

public:
    explicit CollectionProxyModel( QObject* parent = 0 );

protected:
    bool lessThan( const QModelIndex& left, const QModelIndex& right ) const;
};

#endif // COLLECTIONPROXYMODEL_H
