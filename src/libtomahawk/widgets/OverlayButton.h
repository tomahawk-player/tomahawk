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

#ifndef OVERLAYBUTTON_H
#define OVERLAYBUTTON_H

#include <QPushButton>
#include <QAbstractItemView>
#include <QTimer>

#include "DllMacro.h"

class DLLEXPORT OverlayButton : public QPushButton
{
Q_OBJECT
Q_PROPERTY( qreal opacity READ opacity WRITE setOpacity )

public:
    OverlayButton( QWidget* parent );
    ~OverlayButton();

    qreal opacity() const { return m_opacity; }
    void setOpacity( qreal opacity );

    QString text() const { return m_text; }
    void setText( const QString& text );

    bool shown() const;

public slots:
    void show( int timeoutSecs = 0 );
    void hide();

protected:
//    void changeEvent( QEvent* e );
    void paintEvent( QPaintEvent* event );

private:
    QString m_text;
    qreal m_opacity;

    QWidget* m_parent;
    QTimer m_timer;
};

#endif // OVERLAYBUTTON_H
