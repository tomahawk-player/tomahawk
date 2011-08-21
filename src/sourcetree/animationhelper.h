#ifndef ANIMATIONHELPER_H
#define ANIMATIONHELPER_H

#include <QObject>
#include <QModelIndex>
#include <QSize>
#include <QTimer>
#include <QPropertyAnimation>

class AnimationHelper: public QObject
{
    Q_OBJECT
    Q_PROPERTY( QSize size READ size WRITE setSize NOTIFY sizeChanged )

public:
    AnimationHelper( const QModelIndex& index, QObject *parent = 0 );

    QSize originalSize() const { return m_startSize; }
    QSize size() const { return m_size; }

    bool initialized() const;
    void initialize( const QSize& startValue, const QSize& endValue, int duration );

    void setSize( const QSize& size );

    void expand();
    void collapse( bool immediately = false );

    bool partlyExpanded();
    bool fullyExpanded();

signals:
    void sizeChanged();
    void finished( const QModelIndex& index);

private slots:
    void expandTimeout();
    void collapseTimeout();
    void expandAnimationFinished();
    void collapseAnimationFinished();

private:
    QModelIndex m_index;
    QSize m_size;
    QSize m_targetSize;
    QSize m_startSize;

    QTimer m_expandTimer;
    QTimer m_collapseTimer;

    bool m_fullyExpanded;
    bool m_forceClosing;

    QPropertyAnimation *m_expandAnimation;
    QPropertyAnimation *m_collapseAnimation;
};

#endif // ANIMATIONHELPER_H
