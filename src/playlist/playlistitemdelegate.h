#ifndef PLAYLISTITEMDELEGATE_H
#define PLAYLISTITEMDELEGATE_H

#include <QStyledItemDelegate>

class TrackProxyModel;

class PlaylistItemDelegate : public QStyledItemDelegate
{
Q_OBJECT

public:
    PlaylistItemDelegate( QAbstractItemView* parent = 0, TrackProxyModel* proxy = 0 );

    void updateRowSize( const QModelIndex& index );

public slots:
    void setRemovalProgress( unsigned int progress ) { m_removalProgress = progress; }

protected:
    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    QWidget* createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const;

private:
    unsigned int m_removalProgress;
    QAbstractItemView* m_view;
    TrackProxyModel* m_model;
};

#endif // PLAYLISTITEMDELEGATE_H
