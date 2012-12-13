/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012,      Teo Mrnjavac <teo@kde.org>
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

#ifndef BASICHEADER_H
#define BASICHEADER_H

#include <QtGui/QWidget>

#include "DllMacro.h"

class QLabel;
class ElidedLabel;
class QPaintEvent;
class QBoxLayout;

class DLLEXPORT BasicHeader : public QWidget
{
    Q_OBJECT
public:
    explicit BasicHeader( QWidget* parent = 0 );
    virtual ~BasicHeader();

public slots:
    virtual void setCaption( const QString& s );
    virtual void setDescription( const QString& s );
    virtual void setPixmap( const QPixmap& p );

protected:
    virtual void paintEvent( QPaintEvent* event );

    QLabel* m_imageLabel;
    ElidedLabel* m_captionLabel;
    ElidedLabel* m_descriptionLabel;

    QBoxLayout* m_mainLayout;
    QBoxLayout* m_verticalLayout;

    static QPixmap* s_tiledHeader;
};

#endif // BASICHEADER_H
