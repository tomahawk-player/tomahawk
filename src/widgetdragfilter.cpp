#include "widgetdragfilter.h"
#include <qcoreevent.h>
#include <QMouseEvent>
#include <QApplication>
#include <QDebug>
#include <QMenuBar>

WidgetDragFilter::WidgetDragFilter(QObject* parent)
    : QObject( parent )
    , m_dragStarted( false )
{
    Q_ASSERT( parent->isWidgetType() );
    m_target = QWeakPointer<QWidget>(static_cast<QWidget*>(parent));
    m_target.data()->installEventFilter( this );
}

bool WidgetDragFilter::eventFilter(QObject* obj, QEvent* event)
{
    if( m_target.isNull() || m_target.data() != obj ) 
        return false;
    
    if( event->type() == QEvent::MouseButtonPress ) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>( event );
        if( !canDrag( obj, mouseEvent ) )
            return false;
        if( !( mouseEvent->modifiers() == Qt::NoModifier && mouseEvent->button() == Qt::LeftButton ) )
            return false;
        m_dragPoint = mouseEvent->pos();
        m_dragStarted = true;
        return false;
    } else if( event->type() == QEvent::MouseMove ) {
        if( !m_dragStarted )
            return false;
        QMouseEvent* e = static_cast<QMouseEvent* >(event);
        if( !canDrag( obj, e ) ) {
            m_dragStarted = false;
            
            return false;
        }
        if( e->buttons().testFlag( Qt::LeftButton ) ) {
            m_target.data()->window()->move( m_target.data()->window()->pos() + ( e->pos() - m_dragPoint ) );
            return true;
        }
    } else if( event->type() == QEvent::MouseButtonRelease ) {
        m_dragStarted = false;
    }
    return false;
}

/**
 * Make sure we can really drag this widget. Checks inspired by Oxygen's oxygenwindowmanager.cpp
 */
bool WidgetDragFilter::canDrag(QObject* obj, QMouseEvent* ev) const
{
    if( !obj->isWidgetType() )
        return false;
    
    QWidget* w = static_cast< QWidget* >( obj );
    
    if( QWidget::mouseGrabber() )
        return false;
    
    if( w->cursor().shape() != Qt::ArrowCursor )
        return false;
    
    // Now we check various things about the child position and mouse
    QPoint position( ev->pos() );
    QWidget* child = w->childAt( position );
    
    if( child && child->cursor().shape() != Qt::ArrowCursor ) return false;
    
    // Don't want to drag menubars when selecting an action    
    if( QMenuBar* menuBar = qobject_cast<QMenuBar*>( w ) )
    {
        // check if there is an active action
        if( menuBar->activeAction() && menuBar->activeAction()->isEnabled() ) return false;
        
        // check if action at position exists and is enabled
        if( QAction* action = menuBar->actionAt( position ) )
        {
            if( action->isSeparator() ) return true;
            if( action->isEnabled() ) return false;
        }
        // return true in all other cases
        return true;
        
    } 
    
    return true;
}
