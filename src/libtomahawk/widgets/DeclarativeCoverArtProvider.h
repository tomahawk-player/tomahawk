#ifndef DECLARATIVECOVERARTPROVIDER_H
#define DECLARATIVECOVERARTPROVIDER_H


#include "playlist/PlayableProxyModel.h"

#include <QDeclarativeImageProvider>


namespace Tomahawk
{

class DeclarativeCoverArtProvider: public QDeclarativeImageProvider
{
public:
    DeclarativeCoverArtProvider();
    ~DeclarativeCoverArtProvider();

    QPixmap requestPixmap( const QString &id, QSize *size, const QSize &requestedSize );
};

}
#endif // DECLARATIVECOVERARTPROVIDER_H
