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

#ifndef ANIMATEDCOUNTERLABEL_H
#define ANIMATEDCOUNTERLABEL_H

#include <QLabel>
#include <QTimeLine>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QPointer>

#include "DllMacro.h"

class DLLEXPORT AnimatedCounterLabel : public QLabel
{
Q_OBJECT

public:
    explicit AnimatedCounterLabel( QWidget* parent = 0, Qt::WindowFlags f = 0 );

    void setFormat( const QString& f );

public slots:
    void setVisible( bool b );
    void frame( int f );
    void setVal( unsigned int v );
    void showDiff();

private:
    QTimeLine m_timer;

    // what's in the label text:
    unsigned int m_displayed;

    // current value we are storing (and displaying, or animating towards displaying)
    unsigned int m_val, m_oldval;

    QString m_format;
    QPointer<QLabel> m_diff;
};

#endif // ANIMATEDCOUNTERLABEL_H
