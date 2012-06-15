/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ANIMATEDSPLITTER_H
#define ANIMATEDSPLITTER_H

#include <QSplitter>
#include <QTimeLine>

#include "DllMacro.h"

class AnimatedWidget;

class DLLEXPORT AnimatedSplitter : public QSplitter
{
Q_OBJECT

public:
    explicit AnimatedSplitter( QWidget* parent = 0 );

    void show( int index, bool animate = true );
    void hide( int index, bool animate = true );

    void setGreedyWidget( int index );

    void addWidget( QWidget* widget );
    void addWidget( AnimatedWidget* widget );

signals:
    void shown( QWidget*, bool animated );
    void hidden( QWidget*, bool animated );

protected:
    virtual QSplitterHandle* createHandle();

private slots:
    void changeSize( QWidget* child, const QSize& size );
    
    void onShowRequest();
    void onHideRequest();

    void onSizeChanged( const QSize& size );
    void onResizeRequest( const QPoint& delta );
    
private:
    int m_greedyIndex;
};


class DLLEXPORT AnimatedSplitterHandle : public QSplitterHandle
{
Q_OBJECT
public:
    explicit AnimatedSplitterHandle( Qt::Orientation orientation, QSplitter* parent )
        : QSplitterHandle( orientation, parent )
        , m_indexInSplitter( -1 )
        , m_lastCount( -1 )
    {
        setCursor( Qt::ArrowCursor );
    }

    virtual QSize sizeHint() const;

private:
    mutable int m_indexInSplitter;
    mutable int m_lastCount;
};


class DLLEXPORT AnimatedWidget : public QWidget
{
Q_OBJECT
public:
    explicit AnimatedWidget( AnimatedSplitter* parent );
    virtual ~AnimatedWidget();

    QSize hiddenSize() const { return m_hiddenSize; }
    void setHiddenSize( const QSize& size ) { m_hiddenSize = size; emit hiddenSizeChanged(); }

    bool isHidden() const { return m_isHidden; }

public slots:
    virtual void onShown( QWidget*, bool animated );
    virtual void onHidden( QWidget*, bool animated );

    virtual void hide() { emit hideWidget(); }
    virtual void show() { emit showWidget(); }

signals:
    void showWidget();
    void hideWidget();
    
    void animationFinished();

    void resizeBy( const QPoint& delta );
    void sizeChanged( const QSize& size );
    void sizeHintChanged( const QSize& size );
    void hiddenSizeChanged();

private slots:
    void onAnimationStep( int frame );
    void onAnimationFinished();

protected:
    AnimatedSplitter* splitter() { return m_parent; }

private:
    AnimatedSplitter* m_parent;
    bool m_animateForward;
    QSize m_hiddenSize;
    bool m_isHidden;
    QTimeLine* m_timeLine;
};

#endif //ANIMATEDSPLITTER_H
