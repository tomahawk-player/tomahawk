#ifndef SOURCESMODEL_H
#define SOURCESMODEL_H

#include <QStandardItemModel>

#include "source.h"
#include "typedefs.h"

class SourceTreeItem;
class SourceTreeView;

class SourcesModel : public QStandardItemModel
{
Q_OBJECT

public:    
    enum SourceType {
        Invalid = -1,
        
        CollectionSource = 0,
        PlaylistSource = 1,
        DynamicPlaylistSource = 2
    };
    
    explicit SourcesModel( SourceTreeView* parent = 0 );

    virtual QStringList mimeTypes() const;
    virtual Qt::DropActions supportedDropActions() const;
    virtual Qt::ItemFlags flags( const QModelIndex& index ) const;
    QVariant data( const QModelIndex& index, int role ) const;

    bool appendItem( const Tomahawk::source_ptr& source );
    bool removeItem( const Tomahawk::source_ptr& source );

    static SourceType indexType( const QModelIndex& index );
    static Tomahawk::playlist_ptr indexToPlaylist( const QModelIndex& index );
    static Tomahawk::dynplaylist_ptr indexToDynamicPlaylist( const QModelIndex& index );
    static SourceTreeItem* indexToTreeItem( const QModelIndex& index );

signals:
    void clicked( const QModelIndex& );

protected:
    bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );

private slots:
    void onSourceAdded( const QList<Tomahawk::source_ptr>& sources );
    void onSourceAdded( const Tomahawk::source_ptr& source );
    void onSourceRemoved( const Tomahawk::source_ptr& source );

    void onSourceChanged();

    void onItemOnline( const QModelIndex& idx );
    void onItemOffline( const QModelIndex& idx );

public slots:
    void loadSources();

private:
    SourceTreeView* m_parent;
};

#endif // SOURCESMODEL_H
