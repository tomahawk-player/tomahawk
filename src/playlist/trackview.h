#ifndef TRACKVIEW_H
#define TRACKVIEW_H

#include <QTreeView>
#include <QSortFilterProxyModel>

#include "playlistitemdelegate.h"

class PlaylistInterface;
class TrackModel;
class TrackProxyModel;

class TrackView : public QTreeView
{
Q_OBJECT

public:
    explicit TrackView( QWidget* parent = 0 );
    ~TrackView();

    void setProxyModel( TrackProxyModel* model );

    TrackModel* model() { return m_model; }
    TrackProxyModel* proxyModel() { return (TrackProxyModel*)m_proxyModel; }
    PlaylistItemDelegate* delegate() { return m_delegate; }

    void setModel( TrackModel* model );

    QModelIndex contextMenuIndex() const { return m_contextMenuIndex; }
    void setContextMenuIndex( const QModelIndex& idx ) { m_contextMenuIndex = idx; }

public slots:
    void onItemActivated( const QModelIndex& index );

    void playItem();
    void addItemsToQueue();

protected:
    virtual void resizeEvent( QResizeEvent* event );

    virtual void startDrag( Qt::DropActions supportedActions );
    virtual void dragEnterEvent( QDragEnterEvent* event );
    virtual void dragLeaveEvent( QDragLeaveEvent* event ) { m_dragging = false; setDirtyRegion( m_dropRect ); }
    virtual void dragMoveEvent( QDragMoveEvent* event );
    virtual void dropEvent( QDropEvent* event );

    void paintEvent( QPaintEvent* event );
    void keyPressEvent( QKeyEvent* event );

private slots:
    void onItemResized( const QModelIndex& index );

    void resizeColumns();
    void onSectionResized( int logicalIndex, int oldSize, int newSize );

    void onFilterChanged( const QString& filter );

private:
    void restoreColumnsState();
    void saveColumnsState();

    QPixmap createDragPixmap( int itemCount ) const;

    TrackModel* m_model;
    TrackProxyModel* m_proxyModel;
    PlaylistInterface* m_modelInterface;
    PlaylistItemDelegate* m_delegate;

    QList<double> m_columnWeights;
    QList<int> m_columnWidths;

    bool m_resizing;
    bool m_dragging;
    QRect m_dropRect;

    QModelIndex m_contextMenuIndex;
};

#endif // TRACKVIEW_H
