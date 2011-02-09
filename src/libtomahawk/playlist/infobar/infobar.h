#ifndef INFOBAR_H
#define INFOBAR_H

#include <QWidget>

#include "dllmacro.h"

namespace Ui
{
    class InfoBar;
}

class DLLEXPORT InfoBar : public QWidget
{
Q_OBJECT

public:
    InfoBar( QWidget* parent = 0 );
    ~InfoBar();

public slots:
    void setCaption( const QString& s );
    void setDescription( const QString& s );
    void setPixmap( const QPixmap& p );
    
protected:
    void changeEvent( QEvent* e );
    void resizeEvent( QResizeEvent* e );

private:
    Ui::InfoBar* ui;
};

#endif // INFOBAR_H
