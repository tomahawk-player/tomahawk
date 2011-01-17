#ifndef WIDGET_DRAG_FILTER_H
#define WIDGET_DRAG_FILTER_H

#include <QObject>
#include <QPoint>
#include <QWidget>

#include "dllmacro.h"

class QMouseEvent;
class QEvent;

/**
 * This class encapsulates an event filter on a widget that lets any drag events over the widget
 *  translate into move events for the whole application.
 */
class DLLEXPORT WidgetDragFilter : public QObject
{
    Q_OBJECT
public:
    explicit WidgetDragFilter(QObject* parent = 0);  

    virtual bool eventFilter(QObject* , QEvent* );
private:
    bool canDrag( QObject* obj, QMouseEvent* ev ) const;
    
    QWeakPointer<QWidget> m_target; // in case it's deleted under us
    QPoint m_dragPoint;
    bool m_dragStarted;
};

#endif
