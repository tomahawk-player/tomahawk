#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <QStandardItem>

#include "tomahawk/query.h"
#include "tomahawk/result.h"

class PlaylistItem : public QObject
{
Q_OBJECT

public:
//    explicit PlaylistItem() {}
//    explicit PlaylistItem( const PlaylistItem& item ) { m_query = item.query(); m_columns << item.columns(); }
    explicit PlaylistItem( const Tomahawk::query_ptr& query, QObject* parent = 0 );
    explicit PlaylistItem( const Tomahawk::plentry_ptr& entry, QObject* parent = 0 );

    const Tomahawk::plentry_ptr& entry() const { return m_entry; };
    const Tomahawk::query_ptr& query() const { return m_query; };
    QList<QStandardItem*> columns() const { return m_columns; };
    bool beingRemoved() { return m_beingRemoved; }
    QModelIndex index() const;

public slots:
    void setBeingRemoved( bool state );

private slots:
    void onResultsAdded( const QList<Tomahawk::result_ptr>& result );

private:
    void setupItem( const Tomahawk::query_ptr& query );

    Tomahawk::plentry_ptr m_entry;
    Tomahawk::query_ptr m_query;
    QList<QStandardItem*> m_columns;
    bool m_beingRemoved;
};

#endif // PLAYLISTITEM_H
