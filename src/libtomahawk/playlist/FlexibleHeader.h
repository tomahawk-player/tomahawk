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

#ifndef FLEXIBLEHEADER_H
#define FLEXIBLEHEADER_H

#include <QWidget>
#include <QTimer>

#include "DllMacro.h"
#include "Artist.h"

class QPaintEvent;
class FlexibleView;

namespace Ui
{
    class PlaylistHeader;
}

class DLLEXPORT FlexibleHeader : public QWidget
{
Q_OBJECT

public:
    FlexibleHeader( FlexibleView* parent );
    ~FlexibleHeader();

public slots:
    void setCaption( const QString& s );
    void setDescription( const QString& s );
    void setPixmap( const QPixmap& p );

    void setFilter( const QString& filter );

signals:
    void filterTextChanged( const QString& filter );

protected:
    void changeEvent( QEvent* e );
    void paintEvent( QPaintEvent* e );

private slots:
    void onFilterEdited();
    void applyFilter();

private:
    FlexibleView* m_parent;
    Ui::PlaylistHeader* ui;

    QString m_filter;
    QTimer m_filterTimer;

    static QPixmap* s_tiledHeader;
};

#endif
