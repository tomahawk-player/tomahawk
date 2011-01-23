#ifndef OVERLAYWIDGET_H
#define OVERLAYWIDGET_H

#include <QWidget>

#include "dllmacro.h"

class DLLEXPORT OverlayWidget : public QWidget
{
Q_OBJECT

public:
    OverlayWidget();
    ~OverlayWidget();

    QPixmap pixmap();

    QString text() const { return m_text; }
    void setText( const QString& text );

    void paint( QPainter* painter );

protected:
//    void changeEvent( QEvent* e );
    void paintEvent( QPaintEvent* event );

private:
    QString m_text;
    QPixmap m_pixmap;
};

#endif // WELCOMEWIDGET_H
