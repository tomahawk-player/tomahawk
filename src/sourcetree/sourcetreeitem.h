#ifndef SOURCETREEITEM_H
#define SOURCETREEITEM_H

#include <QObject>
#include <QStandardItem>

#include "tomahawk/typedefs.h"
#include "sourcetreeitemwidget.h"
#include "dynamic/DynamicPlaylist.h"

class SourceTreeItem : public QObject
{
Q_OBJECT
public:
    
    enum PlaylistItemType {
        Type = Qt::UserRole + 1, /// Value is SourcesModel::SourceType
        SourceItemPointer = Qt::UserRole + 2, /// value is the sourcetreeritem of the collection itself.
        PlaylistPointer = Qt::UserRole + 3,  /// Value is the playlist_ptr.data()
        DynamicPlaylistPointer = Qt::UserRole + 4 /// Value is the playlist_ptr.data()
    };
    
    
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
    
    void onDynamicPlaylistsAdded( const QList<Tomahawk::dynplaylist_ptr>& playlists );
    void onDynamicPlaylistsDeleted( const QList<Tomahawk::dynplaylist_ptr>& playlists );
    void onDynamicPlaylistsLoaded( Tomahawk::DynamicPlaylistRevision revision );
private:
    void playlistsAdded( const QList<Tomahawk::playlist_ptr>& playlists, bool dynamic );
    void playlistsDeleted( const QList<Tomahawk::playlist_ptr>& playlists, bool dynamic );
    void playlistLoaded( Tomahawk::PlaylistRevision revision, bool dynamic );
    
    QList<QStandardItem*> m_columns;
    Tomahawk::source_ptr m_source;
    SourceTreeItemWidget* m_widget;
    QList<Tomahawk::playlist_ptr> m_playlists;
    QList<Tomahawk::dynplaylist_ptr> m_dynplaylists;

    // playist->guid() -> currently loaded revision
    QMap<QString,QString> m_current_revisions;
    QMap<QString,QString> m_current_dynamic_revisions;
};

#endif // SOURCETREEITEM_H
