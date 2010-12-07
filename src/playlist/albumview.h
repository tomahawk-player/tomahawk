#ifndef ALBUMVIEW_H
#define ALBUMVIEW_H

#include <QListView>
#include <QSortFilterProxyModel>

class AlbumModel;
class AlbumProxyModel;

class AlbumView : public QListView
{
Q_OBJECT

public:
    explicit AlbumView( QWidget* parent = 0 );
    ~AlbumView();

    void setProxyModel( AlbumProxyModel* model );

    AlbumModel* model() { return m_model; }
    AlbumProxyModel* proxyModel() { return m_proxyModel; }
//    PlaylistItemDelegate* delegate() { return m_delegate; }

    void setModel( AlbumModel* model );

public slots:
    void onItemActivated( const QModelIndex& index );

protected:
    virtual void startDrag( Qt::DropActions supportedActions );
    virtual void dragEnterEvent( QDragEnterEvent* event );
    virtual void dragMoveEvent( QDragMoveEvent* event );
    virtual void dropEvent( QDropEvent* event );

    void paintEvent( QPaintEvent* event );

private slots:
    void onFilterChanged( const QString& filter );

private:
    QPixmap createDragPixmap( int itemCount ) const;

    AlbumModel* m_model;
    AlbumProxyModel* m_proxyModel;
//    PlaylistItemDelegate* m_delegate;
};

#endif // ALBUMVIEW_H
