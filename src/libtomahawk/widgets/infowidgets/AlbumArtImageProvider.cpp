#include "AlbumArtImageProvider.h"

#include <QDebug>

QPixmap AlbumArtImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    qDebug() << "*!*!*!*!*!*!*!*!*!* image requested";
    int width = 100;
    int height = 50;

    if (size)
        *size = QSize(width, height);
    QPixmap pixmap(requestedSize.width() > 0 ? requestedSize.width() : width,
                   requestedSize.height() > 0 ? requestedSize.height() : height);
    pixmap.fill(QColor(id).rgba());

    return pixmap;
}
