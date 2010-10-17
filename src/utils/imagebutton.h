#ifndef IMAGE_BUTTON_H
#define IMAGE_BUTTON_H

#include <QAbstractButton>
#include <QIcon>
#include <QMap>


class ImageButton : public QAbstractButton
{
Q_OBJECT

public:
    /** this pixmap becomes the rest state pixmap and defines the size of the eventual widget */
    explicit ImageButton( QWidget* parent = 0 );
    explicit ImageButton( const QPixmap& pixmap, QWidget* parent = 0 );
    explicit ImageButton( const QString& path, QWidget* parent = 0 );
    
    void setPixmap( const QString& path );
    void setPixmap( const QString&, const QIcon::State, QIcon::Mode = QIcon::Normal );
    void setPixmap( const QPixmap&, const QIcon::State = QIcon::Off, QIcon::Mode = QIcon::Normal );
    
    virtual QSize sizeHint() const { return m_sizeHint; }
    
protected:
    virtual void paintEvent( QPaintEvent* event );
    
private:
    void init( const QPixmap& );

    QSize m_sizeHint;
};

#endif //IMAGE_BUTTON_H
