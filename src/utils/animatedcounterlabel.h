#ifndef ANIMATEDCOUNTERLABEL_H
#define ANIMATEDCOUNTERLABEL_H

#include <QLabel>
#include <QTimeLine>
#include <QDebug>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <cmath>

class AnimatedCounterLabel : public QLabel
{
Q_OBJECT

public:
    explicit AnimatedCounterLabel( QWidget* parent = 0, Qt::WindowFlags f = 0 )
        : QLabel( parent, f )
        , m_displayed( 0 )
        , m_val( 0 )
        , m_oldval( 0 )
        , m_format( "%L1" )
    {
        connect( &m_timer, SIGNAL( frameChanged( int ) ), SLOT( frame( int ) ) );
        connect( &m_timer, SIGNAL( finished() ), SLOT( showDiff() ) );
    }

    void setFormat( const QString& f )
    {
        m_format = f;
        setText( m_format.arg( m_displayed ) );
    }

public slots:
    void frame( int f )
    {
        m_displayed = f;
        QLabel::setText( m_format.arg( m_displayed ) );
        QLabel::update();
    }

    void setVal( unsigned int v )
    {
        if( v == m_val )
            return;

        m_oldval = m_val;
        m_val = v;
        m_timer.stop();
        unsigned int dur = 1000;
        unsigned int r = abs( v - m_oldval );

        if( r > 1000 )        dur = 1500;
        else if( r > 10000 )  dur = 2000;
        else if( r > 25000 )  dur = 2250;
        else if( r > 50000 )  dur = 2750;
        else if( r > 100000 ) dur = 3000;
        else if( r > 500000 ) dur = 5000;

        m_timer.setDuration( dur );
        m_timer.setFrameRange( m_displayed, v );
        m_timer.setEasingCurve( QEasingCurve( QEasingCurve::OutCubic ) );
        m_timer.start();
    }

    void showDiff()
    {
        int differ = m_val - m_oldval;
        QLabel* diff = new QLabel( QString("%1 %L2" ).arg( differ > 0 ? "+" : "" )
                                                     .arg( (int)m_val - (int)m_oldval ),
                                                     this->parentWidget() );

        diff->setStyleSheet( "font-size:9px; color:grey;" );
        diff->move( QPoint( this->pos().x(), this->pos().y() ) );
        QPropertyAnimation* a = new QPropertyAnimation( diff, "pos" );
        a->setEasingCurve( QEasingCurve( QEasingCurve::InQuad ) );
        a->setStartValue( diff->pos() + QPoint( 0, -10 ) );
        a->setEndValue( QPoint( diff->pos().x(), diff->pos().y() - 25 ) );
        a->setDuration( 1000 );
        //        qDebug() << "ANIMATING DIFF:" << a->startValue() << a->endValue();

        connect( a, SIGNAL( finished() ), diff, SLOT( hide() ) );
        connect( a, SIGNAL( finished() ), diff, SLOT( deleteLater() ) );
        connect( a, SIGNAL( finished() ), a, SLOT( deleteLater() ) );

        diff->show();
        a->start();
    }

private:
    QTimeLine m_timer;

    // what's in the label text:
    unsigned int m_displayed;

    // current value we are storing (and displaying, or animating towards displaying)
    unsigned int m_val, m_oldval;

    QString m_format;

};

#endif // ANIMATEDCOUNTERLABEL_H
