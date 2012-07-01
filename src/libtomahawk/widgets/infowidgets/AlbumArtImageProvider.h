#include <QDeclarativeImageProvider>

class AlbumArtImageProvider : public QDeclarativeImageProvider
{
public:
    AlbumArtImageProvider()
        : QDeclarativeImageProvider(QDeclarativeImageProvider::Pixmap)
    {
    }

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize);
};
