/*
    Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef SOURCETREEITEM_H
#define SOURCETREEITEM_H

#include "Typedefs.h"
#include "SourcesModel.h"

#include <QIcon>

class QMimeData;

class SourceTreeItem : public QObject
{
    Q_OBJECT
public:
    enum DropType
    {
        DropTypesNone =         0x00,
        DropTypeThisTrack =     0x01,
        DropTypeThisAlbum =     0x02,
        DropTypeAllFromArtist = 0x04,
        DropTypeLocalItems =    0x08,
        DropTypeTop50 =         0x10,
        DropTypesAllTypes =     0xff
    };
    Q_DECLARE_FLAGS( DropTypes, DropType )

    SourceTreeItem() : m_type( SourcesModel::Invalid ), m_parent( 0 ), m_model( 0 ) {}
    SourceTreeItem( SourcesModel* model, SourceTreeItem* parent, SourcesModel::RowType thisType, int peerSortValue = 0, int index = -1 ); // if index is -1, append at end of parent's child list
    virtual ~SourceTreeItem();

    // generic info used by the tree model
    SourcesModel::RowType type() const;
    SourceTreeItem* parent() const;
    SourcesModel* model() const;

    QList< SourceTreeItem* > children() const;
    void appendChild( SourceTreeItem* item );
    void insertChild( int index, SourceTreeItem* item );
    void removeChild( SourceTreeItem* item );

    // varies depending on the type of the item
    virtual QString text() const;
    virtual QString tooltip() const;
    virtual Qt::ItemFlags flags() const;
    virtual QIcon icon() const;
    virtual bool willAcceptDrag( const QMimeData* ) const;
    virtual bool dropMimeData( const QMimeData*, Qt::DropAction );
    virtual bool setData( const QVariant&, bool );
    virtual int peerSortValue() const; // How to sort relative to peers in the tree.
    virtual int IDValue() const;
    virtual DropTypes supportedDropTypes( const QMimeData* mimeData ) const;
    virtual void setDropType( DropType type );
    virtual DropType dropType() const;
    virtual bool isBeingPlayed() const;
    virtual QList< QAction* > customActions() const;

    /// don't call me unless you are a sourcetreeitem. i prefer this to making everyone a friend
    void beginRowsAdded( int from, int to );
    void endRowsAdded();
    void beginRowsRemoved( int from, int to );
    void endRowsRemoved();

public slots:
    virtual void activate() {}
    virtual void doubleClicked() {}

signals:
    void updated();
    void selectRequest( SourceTreeItem* );
    void expandRequest( SourceTreeItem* );
    void toggleExpandRequest( SourceTreeItem* );

    void beginChildRowsAdded( int fromRow, int toRow );
    void childRowsAdded();

    void beginChildRowsRemoved( int fromRow, int toRow );
    void childRowsRemoved();

protected:
    void setRowType( SourcesModel::RowType t );
    void setParentItem( SourceTreeItem* item );

private:
    SourcesModel::RowType m_type;

    SourceTreeItem* m_parent;
    QList< SourceTreeItem* > m_children;
    SourcesModel* m_model;
    int m_peerSortValue;

    DropType m_dropType;
};

Q_DECLARE_METATYPE( SourceTreeItem* );

#endif // SOURCETREEITEM_H
