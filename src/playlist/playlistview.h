#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QHeaderView>
#include <QTreeView>
#include <QTimer>

#include "tomahawk/source.h"
#include "tomahawk/playlistmodelinterface.h"
#include "progresstreeview.h"
#include "playlistitem.h"
#include "playlistitemdelegate.h"

class PlaylistModel;
class PlaylistProxyModel;

class PlaylistView : public ProgressTreeView
{
Q_OBJECT

public:
    explicit PlaylistView( QWidget* parent = 0 );
    ~PlaylistView();

    PlaylistProxyModel* model() { return m_proxyModel; }
    PlaylistModel* playlistModel() { return m_model; }
    PlaylistItemDelegate* delegate() { return m_delegate; }

    void setModel( PlaylistModel* model );

signals:
    void itemStarted( PlaylistModelInterface* model, PlaylistItem* item );
    void playlistModelChanged( PlaylistModelInterface* model );

    void numSourcesChanged( unsigned int i );
    void numTracksChanged( unsigned int i );
    void numArtistsChanged( unsigned int i );
    void numShownChanged( unsigned int i );

public slots:
    void addSource( const Tomahawk::source_ptr& source );
    void removeSource( const Tomahawk::source_ptr& source );

    void setFilter( const QString& filter );

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
    void onAudioStopped();
    void applyFilter();

    void resizeColumns();
    void onSectionResized( int logicalIndex, int oldSize, int newSize );

private:
    void restoreColumnsState();
    void saveColumnsState();

    PlaylistProxyModel* m_proxyModel;
    PlaylistModel* m_model;
    PlaylistItemDelegate* m_delegate;

    QTimer m_filterTimer;
    QString m_filter;

    QList<double> m_columnWeights;
    QList<int> m_columnWidths;

    bool m_resizing;
    bool m_dragging;
    QRect m_dropRect;
};

#endif // PLAYLISTVIEW_H
