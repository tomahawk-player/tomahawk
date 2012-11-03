/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Teo Mrnjavac <teo@kde.org>
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

#ifndef UNSTYLEDFRAME_H
#define UNSTYLEDFRAME_H

#include <QWidget>

/**
 * @brief The UnstyledFrame class is just a QWidget with an overridden paintEvent
 * to provide a *really* unstyled frame to be used with styles that don't obey
 * QFrame::Shape.
 */
class UnstyledFrame : public QWidget
{
    Q_OBJECT
public:
    explicit UnstyledFrame( QWidget* parent = 0 );

    void setFrameColor( const QColor& color );

protected:
    void paintEvent( QPaintEvent* event );

private:
    QColor m_frameColor;
};

#endif // UNSTYLEDFRAME_H
