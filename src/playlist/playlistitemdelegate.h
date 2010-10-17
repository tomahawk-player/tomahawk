#ifndef PLAYLISTITEMDELEGATE_H
#define PLAYLISTITEMDELEGATE_H

#include <QStyledItemDelegate>

class PlaylistItemDelegate : public QStyledItemDelegate
{
Q_OBJECT

public:
    PlaylistItemDelegate( QObject* parent = 0 );

    void updateRowSize( const QModelIndex& index );

public slots:
    void setRemovalProgress( unsigned int progress ) { m_removalProgress = progress; }

protected:
    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

private:
    unsigned int m_removalProgress;
};

#endif // PLAYLISTITEMDELEGATE_H
