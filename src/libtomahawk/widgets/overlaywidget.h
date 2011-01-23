#ifndef OVERLAYWIDGET_H
#define OVERLAYWIDGET_H

#include <QWidget>
#include <QAbstractItemView>

#include "dllmacro.h"

class DLLEXPORT OverlayWidget : public QWidget
{
Q_OBJECT
Q_PROPERTY( qreal opacity READ opacity WRITE setOpacity )

public:
    OverlayWidget( QAbstractItemView* parent );
    ~OverlayWidget();

    QPixmap pixmap();

    qreal opacity() const { return m_opacity; }
    void setOpacity( qreal opacity );

    QString text() const { return m_text; }
    void setText( const QString& text );

    void paint( QPainter* painter );

protected:
//    void changeEvent( QEvent* e );
    void paintEvent( QPaintEvent* event );

private:
    QString m_text;
    QPixmap m_pixmap;
    qreal m_opacity;

    QAbstractItemView* m_parent;
};

#endif // WELCOMEWIDGET_H
