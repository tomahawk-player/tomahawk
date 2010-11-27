#ifndef QUEUEVIEW_H
#define QUEUEVIEW_H

#include <QPushButton>

#include "playlistview.h"

class QueueView : public QWidget
{
Q_OBJECT

public:
    explicit QueueView( QWidget* parent = 0 );
    ~QueueView();

    PlaylistView* queue() const { return m_queue; }

    QSize sizeHint() const { return QSize( 0, 200 ); }

public slots:
    void onShown( QWidget* );
    void onHidden( QWidget* );

signals:
    void showWidget();
    void hideWidget();

private:
    PlaylistView* m_queue;
    QPushButton* m_button;
};

#endif // QUEUEVIEW_H
