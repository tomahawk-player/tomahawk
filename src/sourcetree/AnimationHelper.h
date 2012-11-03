/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Michael Zanetti <mzanetti@kde.org>
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

    QSize originalSize() const;
    QSize size() const;

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

    QPropertyAnimation *m_expandAnimation;
    QPropertyAnimation *m_collapseAnimation;
};

#endif // ANIMATIONHELPER_H
