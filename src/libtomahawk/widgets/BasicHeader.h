/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "ui_HeaderWidget.h"

#include "utils/DpiScaler.h"
#include "widgets/BackgroundWidget.h"
#include "DllMacro.h"

class QPushButton;
class QLabel;
class ElidedLabel;
class QPaintEvent;
class QBoxLayout;

class DLLEXPORT BasicHeader : public BackgroundWidget, private TomahawkUtils::DpiScaler
{
    Q_OBJECT
public:
    explicit BasicHeader( QWidget* parent = 0 );
    virtual ~BasicHeader();

    QScopedPointer<Ui::HeaderWidget> ui;

    QPushButton* addButton( const QString& text );

public slots:
    virtual void setCaption( const QString& s );
    virtual void setDescription( const QString& s );
    virtual void setPixmap( const QPixmap& p, bool tinted = true );
    virtual void setRefreshVisible( bool visible );

signals:
    void refresh();

protected:
    virtual void resizeEvent( QResizeEvent* event );
};

#endif // BASICHEADER_H
