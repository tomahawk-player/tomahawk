#include "infobar.h"
#include "ui_infobar.h"

#include <QLabel>
#include <QPropertyAnimation>
#include <QPixmap>

#include "utils/tomahawkutils.h"


InfoBar::InfoBar( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::InfoBar )
{
    ui->setupUi( this );
    layout()->setSpacing( 0 );
    layout()->setContentsMargins( 0, 0, 0, 0 );

    QFont boldFont = ui->captionLabel->font();
    boldFont.setPixelSize( 18 );
    boldFont.setBold( true );
    ui->captionLabel->setFont( boldFont );

    boldFont.setPixelSize( 12 );
    ui->descriptionLabel->setFont( boldFont );
    ui->descriptionLabel->setMargin( 2 );

    QPalette whitePal = ui->captionLabel->palette();
    whitePal.setColor( QPalette::Foreground, Qt::white );

    ui->captionLabel->setPalette( whitePal );
    ui->descriptionLabel->setPalette( whitePal );
    
    setAutoFillBackground( true );
}


InfoBar::~InfoBar()
{
    delete ui;
}


void
InfoBar::setCaption( const QString& s )
{
    ui->captionLabel->setText( s );
}


void
InfoBar::setDescription( const QString& s )
{
    ui->descriptionLabel->setText( s );
}


void
InfoBar::setPixmap( const QPixmap& p )
{
    ui->imageLabel->setPixmap( p.scaledToHeight( ui->imageLabel->height(), Qt::SmoothTransformation ) );
}


void
InfoBar::changeEvent( QEvent* e )
{
    QWidget::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}


void
InfoBar::resizeEvent( QResizeEvent* e )
{
    QWidget::resizeEvent( e );

    QLinearGradient gradient = QLinearGradient( contentsRect().topLeft(), contentsRect().bottomRight() );
    gradient.setColorAt( 0.0, QColor( 100, 100, 100 ) );
    gradient.setColorAt( 1.0, QColor( 63, 63, 63 ) );

    QPalette p = palette();
    p.setBrush( QPalette::Window, QBrush( gradient ) );
    setPalette( p );
}
