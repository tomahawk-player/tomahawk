#ifndef ALBUMITEM_H
#define ALBUMITEM_H

#include <QAbstractItemModel>
#include <QHash>
#include <QPersistentModelIndex>
#include <QPixmap>

#include "album.h"

class AlbumItem : public QObject
{
Q_OBJECT

public:
    ~AlbumItem();

    explicit AlbumItem( AlbumItem* parent = 0, QAbstractItemModel* model = 0 );
    explicit AlbumItem( const Tomahawk::album_ptr& album, AlbumItem* parent = 0, int row = -1 );

    const Tomahawk::album_ptr& album() const { return m_album; };
    void setCover( const QPixmap& cover ) { this->cover = cover; emit dataChanged(); }

    AlbumItem* parent;
    QList<AlbumItem*> children;
    QHash<QString, AlbumItem*> hash;
    int childCount;
    QPersistentModelIndex index;
    QAbstractItemModel* model;
    QPixmap cover;
    bool toberemoved;

signals:
    void dataChanged();

private:
    Tomahawk::album_ptr m_album;
};

#endif // ALBUMITEM_H
