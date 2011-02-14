#ifndef TRACKHEADER_H
#define TRACKHEADER_H

#include <QHeaderView>
#include <QSignalMapper>

#include "dllmacro.h"

class TrackView;

class DLLEXPORT TrackHeader : public QHeaderView
{
Q_OBJECT

public:
    explicit TrackHeader( TrackView* parent = 0 );
    ~TrackHeader();

    int visibleSectionCount() const;

public slots:
    void toggleVisibility( int index );
    void checkState();

protected:
    void contextMenuEvent( QContextMenuEvent* e );

private slots:
    void onSectionResized();
    void onToggleResizeColumns();

private:
    void addColumnToMenu( int index );

    TrackView* m_parent;

    QMenu* m_menu;
    QSignalMapper* m_sigmap;
    QList<QAction*> m_visActions;
    bool m_init;
};

#endif
