#ifndef ANIMATEDSPLITTER_H
#define ANIMATEDSPLITTER_H

#include <QSplitter>
#include <QTimeLine>

class AnimatedWidget;

class AnimatedSplitter : public QSplitter
{
Q_OBJECT

public:
    explicit AnimatedSplitter( QWidget* parent = 0 );

    void show( int index, bool animate = true );
    void hide( int index, bool animate = true );

    void setGreedyWidget( int index ) { m_greedyIndex = index; }

    void addWidget( QWidget* widget );
    void addWidget( AnimatedWidget* widget );

signals:
    void shown( QWidget* );
    void hidden( QWidget* );

private slots:
    void onShowRequest();
    void onHideRequest();

    void onAnimationStep( int frame );
    void onAnimationFinished();

    void onHiddenSizeChanged();

private:
    int m_animateIndex;
    bool m_animateForward;

    int m_greedyIndex;
    QList<QSize> m_sizes;
    QTimeLine* m_timeLine;
};

class AnimatedWidget : public QWidget
{
Q_OBJECT
public:
    explicit AnimatedWidget( AnimatedSplitter* parent );

    QSize hiddenSize() const { return m_hiddenSize; }
    void setHiddenSize( const QSize& size ) { m_hiddenSize = size; emit hiddenSizeChanged(); }

public slots:
    virtual void onShown( QWidget* ) {}
    virtual void onHidden( QWidget* ) {}

signals:
    void showWidget();
    void hideWidget();

    void hiddenSizeChanged();

private:
    AnimatedSplitter* m_parent;
    QSize m_hiddenSize;
};

#endif //ANIMATEDSPLITTER_H
