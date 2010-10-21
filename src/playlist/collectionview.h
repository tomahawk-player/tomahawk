#ifndef COLLECTIONVIEW_H
#define COLLECTIONVIEW_H

#include <QHeaderView>
#include <QTreeView>
#include <QTimer>
#include <QSortFilterProxyModel>

#include "tomahawk/source.h"
#include "playlist/trackmodel.h"
#include "plitem.h"
#include "playlistitemdelegate.h"

class CollectionProxyModel;
class TrackProxyModel;
class PlaylistInterface;

class CollectionView : public QTreeView
{
Q_OBJECT

public:
    explicit CollectionView( QWidget* parent = 0 );
    ~CollectionView();

    TrackModel* model() { return m_model; }
    TrackProxyModel* proxyModel() { return (TrackProxyModel*)m_proxyModel; }
    PlaylistItemDelegate* delegate() { return m_delegate; }

    void setModel( TrackModel* model, PlaylistInterface* modelInterface );

protected:
    virtual void resizeEvent( QResizeEvent* event );
    virtual void keyPressEvent( QKeyEvent* event );

    virtual void dragEnterEvent( QDragEnterEvent* event );
    virtual void dragLeaveEvent( QDragLeaveEvent* event ) { m_dragging = false; setDirtyRegion( m_dropRect ); }
    virtual void dragMoveEvent( QDragMoveEvent* event );
    virtual void dropEvent( QDropEvent* event );

    void paintEvent( QPaintEvent* event );

private slots:
    void onItemActivated( const QModelIndex& index );
    void onItemResized( const QModelIndex& index );

    void resizeColumns();
    void onSectionResized( int logicalIndex, int oldSize, int newSize );

    void onFilterChanged( const QString& filter );

private:
    void restoreColumnsState();
    void saveColumnsState();

    TrackModel* m_model;
    PlaylistInterface* m_modelInterface;
    CollectionProxyModel* m_proxyModel;
    PlaylistItemDelegate* m_delegate;

    QList<double> m_columnWeights;
    QList<double> m_artistColumnWeights;
    QList<int> m_columnWidths;
    QList<int> m_artistColumnWidths;

    bool m_resizing;
    bool m_dragging;
    QRect m_dropRect;
};

#endif // COLLECTIONVIEW_H
