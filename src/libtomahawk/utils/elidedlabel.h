#ifndef ELIDEDLABEL_H
#define ELIDEDLABEL_H

#include <QFrame>
#include <QTime>

#include "dllmacro.h"

class DLLEXPORT ElidedLabel : public QFrame
{
    Q_OBJECT
    Q_PROPERTY( QString text READ text WRITE setText NOTIFY textChanged )
    Q_PROPERTY( Qt::Alignment alignment READ alignment WRITE setAlignment )
    Q_PROPERTY( Qt::TextElideMode elideMode READ elideMode WRITE setElideMode )

public:
    explicit ElidedLabel( QWidget* parent = 0, Qt::WindowFlags flags = 0 );
    explicit ElidedLabel( const QString& text, QWidget* parent = 0, Qt::WindowFlags flags = 0 );
    virtual ~ElidedLabel();

    QString text() const;

    Qt::Alignment alignment() const;
    void setAlignment( Qt::Alignment alignment );

    Qt::TextElideMode elideMode() const;
    void setElideMode( Qt::TextElideMode mode );

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void init( const QString& txt = QString() );
    void updateLabel();

public slots:
    void setText( const QString& text );

signals:
    void clicked();
    void textChanged( const QString& text );

protected:
    virtual void changeEvent( QEvent* event );
    virtual void mousePressEvent( QMouseEvent* event );
    virtual void mouseReleaseEvent( QMouseEvent* event );
    virtual void paintEvent( QPaintEvent* event );
    
private:
    QTime time;
    QString m_text;
    Qt::Alignment align;
    Qt::TextElideMode mode;
};

#endif // ELIDEDLABEL_H
