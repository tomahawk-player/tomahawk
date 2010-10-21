#ifndef PLITEM_H
#define PLITEM_H

#include <QHash>
#include <QPersistentModelIndex>

#include "tomahawk/query.h"
#include "tomahawk/result.h"

class PlItem : public QObject
{
Q_OBJECT

public:
    ~PlItem();

    explicit PlItem( PlItem* parent = 0 );
    explicit PlItem( const QString& caption, PlItem* parent = 0 );
    explicit PlItem( const Tomahawk::query_ptr& query, PlItem* parent = 0 );
    explicit PlItem( const Tomahawk::plentry_ptr& entry, PlItem* parent = 0 );

    const Tomahawk::plentry_ptr& entry() const { return m_entry; };
    const Tomahawk::query_ptr& query() const { return m_query; };

    bool isPlaying() { return m_isPlaying; }
    void setIsPlaying( bool b ) { m_isPlaying = b; emit dataChanged(); }

    PlItem* parent;
    QList<PlItem*> children;
    QHash<QString, PlItem*> hash;
    QString caption;
    int childCount;
    QModelIndex index;

signals:
    void dataChanged();

private slots:
    void onResultsAdded( const QList<Tomahawk::result_ptr>& result );

private:
    void setupItem( const Tomahawk::query_ptr& query, PlItem* parent );

    Tomahawk::plentry_ptr m_entry;
    Tomahawk::query_ptr m_query;
    bool m_isPlaying;
};

#endif // PLITEM_H
