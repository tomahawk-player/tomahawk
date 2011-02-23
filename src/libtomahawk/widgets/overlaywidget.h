#ifndef OVERLAYWIDGET_H
#define OVERLAYWIDGET_H

#include <QWidget>
#include <QAbstractItemView>

#include "dllmacro.h"
#include <QTimer>

class DLLEXPORT OverlayWidget : public QWidget
{
Q_OBJECT
Q_PROPERTY( qreal opacity READ opacity WRITE setOpacity )

public:
    OverlayWidget( QWidget* parent );
    ~OverlayWidget();

    qreal opacity() const { return m_opacity; }
    void setOpacity( qreal opacity );

    QString text() const { return m_text; }
    void setText( const QString& text );

    bool shown() const;
public slots:
    void show( int timeoutSecs = 0 );
    void hide();

protected:
//    void changeEvent( QEvent* e );
    void paintEvent( QPaintEvent* event );

private:
    QString m_text;
    qreal m_opacity;

    QWidget* m_parent;
    QTimer m_timer;
};

#endif // OVERLAYWIDGET_H
