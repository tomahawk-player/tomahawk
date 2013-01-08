/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011 - 2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef FADINGPIXMAP_H
#define FADINGPIXMAP_H

#include "utils/SharedTimeLine.h"
#include "utils/TomahawkUtils.h"

#include <QLabel>
#include <QPaintEvent>
#include <QTimeLine>
#include <QPointer>

#include "DllMacro.h"

/**
 * \class FadingPixmap
 * \brief Fades to the new image when calling setPixmap.
 */
class DLLEXPORT FadingPixmap : public QLabel
{
Q_OBJECT

    static QPointer< TomahawkUtils::SharedTimeLine > stlInstance();

public:
    FadingPixmap( QWidget* parent = 0 );
    virtual ~FadingPixmap();

public slots:
    virtual void setPixmap( const QPixmap& pixmap, bool isDefault );
    void onAnimationStep( int frame );

signals:
    void clicked();

protected:
    virtual void paintEvent( QPaintEvent* );
    void mouseReleaseEvent( QMouseEvent* event );

private slots:
    void onAnimationFinished();

private:
    QPixmap m_pixmap;
    QPixmap m_oldPixmap;

    QString m_oldImageMd5;
    
    QList<QPixmap> m_pixmapQueue;
    
    int m_fadePct;
    int m_startFrame;
    bool m_isDefault;

    static QPointer< TomahawkUtils::SharedTimeLine > s_stlInstance;
};

#endif
