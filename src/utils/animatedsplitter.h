#ifndef ANIMATEDSPLITTER_H
#define ANIMATEDSPLITTER_H

#include <QSplitter>

class AnimatedSplitter : public QSplitter
{
Q_OBJECT

public:
    explicit AnimatedSplitter( QWidget* parent = 0 );

    void show( int index, bool animate = true );
    void hide( int index, bool animate = true );

    void setGreedyWidget( int index ) { m_greedyIndex = index; }

    void addWidget( QWidget* widget );

signals:
    void shown( QWidget* );
    void hidden( QWidget* );

private slots:
    void onShowRequest();
    void onHideRequest();

    void onAnimationStep( int frame );
    void onAnimationFinished();

private:
    int m_animateIndex;

    int m_greedyIndex;
    int m_greedyHeight;
};

#endif //ANIMATEDSPLITTER_H
