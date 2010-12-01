#ifndef SOURCETREEITEM_H
#define SOURCETREEITEM_H

#include <QObject>
#include <QStandardItem>

#include "tomahawk/typedefs.h"
#include "sourcetreeitemwidget.h"

class SourceTreeItem : public QObject
{
Q_OBJECT

public:
    explicit SourceTreeItem( const Tomahawk::source_ptr& source, QObject* parent );
    virtual ~SourceTreeItem();

    const Tomahawk::source_ptr& source() const { return m_source; };
    QList<QStandardItem*> columns() const { return m_columns; };

    QWidget* widget() const { return m_widget; };

    // returns revision ID we are curently displaying for given playlist ID
    QString currentlyLoadedPlaylistRevision( const QString& plguid ) const
    {
        return m_current_revisions.value( plguid );
    }

signals:
    void clicked( const QModelIndex& index );

public slots:
    void onOnline();
    void onOffline();

private slots:
    void onClicked();

    void onPlaylistsAdded( const QList<Tomahawk::playlist_ptr>& playlists );
    void onPlaylistsDeleted( const QList<Tomahawk::playlist_ptr>& playlists );
    void onPlaylistLoaded( Tomahawk::PlaylistRevision revision );

private:
    QList<QStandardItem*> m_columns;
    Tomahawk::source_ptr m_source;
    SourceTreeItemWidget* m_widget;
    QList<Tomahawk::playlist_ptr> m_playlists;

    // playist->guid() -> currently loaded revision
    QMap<QString,QString> m_current_revisions;
};

#endif // SOURCETREEITEM_H
