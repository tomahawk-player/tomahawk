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

public slots:
    void showQueue();
    void hideQueue();

private slots:
    void onAnimationStep( int frame );
    void onAnimationFinished();

private:
    PlaylistView* m_queue;
    QPushButton* m_button;
    unsigned int m_prevHeight;
};

#endif // QUEUEVIEW_H
