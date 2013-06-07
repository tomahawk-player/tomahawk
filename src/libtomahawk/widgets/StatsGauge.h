/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef STATS_GAUGE_H
#define STATS_GAUGE_H

#include "DllMacro.h"

#include <QProgressBar>

class DLLEXPORT StatsGauge : public QProgressBar
{
Q_OBJECT
Q_PROPERTY( float percentage READ percentage WRITE setPercentage )

public:
    /** this pixmap becomes the rest state pixmap and defines the size of the eventual widget */
    explicit StatsGauge( QWidget* parent = 0 );

    virtual QSize sizeHint() const { return m_sizeHint; }
    QString text() const { return m_text; }

    float percentage() const { return m_percentage; }

public slots:
    void setValue( int value );
    void setText( const QString& text );
    void setPercentage( float percentage );

protected:
    virtual void paintEvent( QPaintEvent* event );

private:
    QSize m_sizeHint;
    QString m_text;
    float m_percentage;
    int m_targetValue;
};

#endif //STATS_GAUGE_H
