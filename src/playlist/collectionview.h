#ifndef COLLECTIONVIEW_H
#define COLLECTIONVIEW_H

#include "tomahawk/tomahawkapp.h"
#include "trackview.h"

class CollectionView : public TrackView
{
Q_OBJECT

public:
    explicit CollectionView( QWidget* parent = 0 );
    ~CollectionView();
};

#endif // COLLECTIONVIEW_H
