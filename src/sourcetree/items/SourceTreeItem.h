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

#include <QIcon>

#include "Typedefs.h"
#include "SourcesModel.h"

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
    SourcesModel::RowType type() const { return m_type; }
    SourceTreeItem* parent() const { return m_parent; }
    SourcesModel* model() const { return m_model; }

    QList< SourceTreeItem* > children() const { return m_children; }
    void appendChild( SourceTreeItem* item ) { m_children.append( item ); }
    void insertChild( int index, SourceTreeItem* item ) { m_children.insert( index, item ); }
    void removeChild( SourceTreeItem* item ) { m_children.removeAll( item ); }

    // varies depending on the type of the item
    virtual QString text() const { return QString(); }
    virtual QString tooltip() const { return QString(); }
    virtual Qt::ItemFlags flags() const { return Qt::ItemIsSelectable | Qt::ItemIsEnabled; }
    virtual QIcon icon() const { return QIcon(); }
    virtual bool willAcceptDrag( const QMimeData* ) const { return false; }
    virtual bool dropMimeData( const QMimeData*, Qt::DropAction ) { return false; }
    virtual bool setData( const QVariant&, bool ) { return false; }
    virtual int peerSortValue() const { return m_peerSortValue; } // How to sort relative to peers in the tree.
    virtual int IDValue() const { return 0; }
    virtual DropTypes supportedDropTypes( const QMimeData* mimeData ) const { Q_UNUSED( mimeData ); return DropTypesNone; }
    virtual void setDropType( DropType type ) { m_dropType = type; }
    virtual DropType dropType() const { return m_dropType; }
    virtual bool isBeingPlayed() const { return false; }
    virtual QList< QAction* > customActions() const { return QList< QAction* >(); }

    /// don't call me unless you are a sourcetreeitem. i prefer this to making everyone a friend
    void beginRowsAdded( int from, int to ) { emit beginChildRowsAdded( from, to ); }
    void endRowsAdded() { emit childRowsAdded(); }
    void beginRowsRemoved( int from, int to ) { emit beginChildRowsRemoved( from, to ); }
    void endRowsRemoved() { emit childRowsRemoved(); }

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
    void setRowType( SourcesModel::RowType t ) { m_type = t; }
    void setParentItem( SourceTreeItem* item ) { m_parent = item; }

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
