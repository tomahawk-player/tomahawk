#ifndef DECLARATIVECOVERARTPROVIDER_H
#define DECLARATIVECOVERARTPROVIDER_H


#include "playlist/PlayableProxyModel.h"

#include <QDeclarativeImageProvider>


namespace Tomahawk
{

class DeclarativeCoverArtProvider: public QDeclarativeImageProvider
{
public:
    DeclarativeCoverArtProvider( PlayableProxyModel *model );
    ~DeclarativeCoverArtProvider();

    QPixmap requestPixmap( const QString &id, QSize *size, const QSize &requestedSize );

private:
    PlayableProxyModel *m_model;
};

}
#endif // DECLARATIVECOVERARTPROVIDER_H
