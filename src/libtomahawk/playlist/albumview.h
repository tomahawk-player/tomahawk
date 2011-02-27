#ifndef ALBUMVIEW_H
#define ALBUMVIEW_H

#include <QListView>
#include <QSortFilterProxyModel>

#include "albummodel.h"
#include "albumproxymodel.h"
#include "viewpage.h"

#include "dllmacro.h"

class DLLEXPORT AlbumView : public QListView, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    explicit AlbumView( QWidget* parent = 0 );
    ~AlbumView();

    void setProxyModel( AlbumProxyModel* model );

    AlbumModel* model() const { return m_model; }
    AlbumProxyModel* proxyModel() const { return m_proxyModel; }
//    PlaylistItemDelegate* delegate() { return m_delegate; }

    void setModel( AlbumModel* model );

    virtual QWidget* widget() { return this; }
    virtual PlaylistInterface* playlistInterface() const { return proxyModel(); }

    virtual QString title() const { return m_model->title(); }
    virtual QString description() const { return m_model->description(); }

    virtual bool showModes() const { return true; }

    virtual bool jumpToCurrentTrack() { return false; }

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
