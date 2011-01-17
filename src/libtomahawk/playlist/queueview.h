#ifndef QUEUEVIEW_H
#define QUEUEVIEW_H

#include <QPushButton>

#include "utils/animatedsplitter.h"
#include "playlistview.h"

#include "dllmacro.h"

class DLLEXPORT QueueView : public AnimatedWidget
{
Q_OBJECT

public:
    explicit QueueView( AnimatedSplitter* parent );
    ~QueueView();

    PlaylistView* queue() const { return m_queue; }

    QSize sizeHint() const { return QSize( 0, 200 ); }

public slots:
    virtual void onShown( QWidget* );
    virtual void onHidden( QWidget* );

private:
    PlaylistView* m_queue;
    QPushButton* m_button;
};

#endif // QUEUEVIEW_H
