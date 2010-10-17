#ifndef SOURCESMODEL_H
#define SOURCESMODEL_H

#include <QStandardItemModel>

#include "tomahawk/source.h"
#include "tomahawk/typedefs.h"

class SourceTreeItem;

class SourcesModel : public QStandardItemModel
{
Q_OBJECT

public:
    explicit SourcesModel( QObject* parent = 0 );

    virtual QStringList mimeTypes() const;
    virtual Qt::DropActions supportedDropActions() const;
    virtual Qt::ItemFlags flags( const QModelIndex& index ) const;

    bool appendItem( const Tomahawk::source_ptr& source );
    bool removeItem( const Tomahawk::source_ptr& source );

    static int indexType( const QModelIndex& index );
    static Tomahawk::playlist_ptr indexToPlaylist( const QModelIndex& index );
    static SourceTreeItem* indexToTreeItem( const QModelIndex& index );

signals:
    void clicked( const QModelIndex& );

private slots:
    void onSourceAdded( const Tomahawk::source_ptr& source );
    void onSourceRemoved( const Tomahawk::source_ptr& source );

    void onItemOnline( const QModelIndex& idx );
    void onItemOffline( const QModelIndex& idx );

public slots:
    void loadSources();
};

#endif // SOURCESMODEL_H
