/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef IMAGE_BUTTON_H
#define IMAGE_BUTTON_H

#include "DllMacro.h"

#include <QAbstractButton>
#include <QIcon>
#include <QMap>

class DLLEXPORT ImageButton : public QAbstractButton
{
Q_OBJECT

public:
    /** this pixmap becomes the rest state pixmap and defines the size of the eventual widget */
    explicit ImageButton( QWidget* parent = 0 );
    explicit ImageButton( const QPixmap& pixmap, QWidget* parent = 0 );
    explicit ImageButton( const QString& path, QWidget* parent = 0 );

    void setPixmap( const QString& path );
    void setPixmap( const QPixmap& pixmap );

    void setPixmap( const QString&, const QIcon::State, QIcon::Mode = QIcon::Normal );
    void setPixmap( const QPixmap&, const QIcon::State, QIcon::Mode = QIcon::Normal );

    void clear();

    virtual QSize sizeHint() const { return m_sizeHint; }

protected:
    virtual void paintEvent( QPaintEvent* event );

private:
    void init( const QPixmap& );

    QSize m_sizeHint;
};

#endif //IMAGE_BUTTON_H
