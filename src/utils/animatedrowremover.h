#ifndef ANIMATEDROWREMOVER_H
#define ANIMATEDROWREMOVER_H

#include <QTimeLine>
#include <QList>

#include "playlistitemdelegate.h"
#include "playlistmodel.h"
#include "playlistitem.h"
#include "playlistview.h"

class AnimatedRowRemover : public QTimeLine
{
Q_OBJECT

public:
    AnimatedRowRemover( PlaylistView* view, PlaylistModel* model, const QList<PlaylistItem*>& items, QObject* parent = 0 );
    ~AnimatedRowRemover();

public slots:
    void start( bool animate = true );
    void trigger( int step );

private slots:
    void onFinished();

private:
    PlaylistView* m_view;
    PlaylistModel* m_model;
    QList<PlaylistItem*> m_items;
};

#endif // ANIMATEDROWREMOVER_H
