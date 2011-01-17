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

public slots:
    void onResized();
    void toggleVisibility( int index );

protected:
    void contextMenuEvent( QContextMenuEvent* e );

private slots:
    void onSectionResized( int logicalIndex, int oldSize, int newSize );

    void onToggleResizeColumns();

private:
    void addColumnToMenu( int index );

    void restoreColumnsState();
    void saveColumnsState();

    TrackView* m_parent;

    QMenu* m_menu;
    QSignalMapper* m_sigmap;
    QList<QAction*> m_visActions;

    QList<double> m_columnWeights;
    int m_hiddenWidth;
    double m_hiddenPct;
    bool m_init;
};

#endif
