/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012,      Leo Franchi            <lfranchi@kde.org>
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

#ifndef INFOBAR_H
#define INFOBAR_H

#include <QWidget>

#include "DllMacro.h"
#include "Artist.h"

class QueryLabel;
class QCheckBox;
class QTimeLine;
class QSearchField;
class ContextWidget;

namespace Ui
{
    class InfoBar;
}

namespace Tomahawk
{
class PlaylistUpdaterInterface;
}

class DLLEXPORT InfoBar : public QWidget
{
Q_OBJECT

public:
    InfoBar( QWidget* parent = 0 );
    ~InfoBar();

public slots:
    void setCaption( const QString& s );

    void setDescription( const QString& s );
    // If you want a querylabel instead of an ElidedLabel
    void setDescription( const Tomahawk::artist_ptr& artist );
    void setDescription( const Tomahawk::album_ptr& album_ptr );

    void setLongDescription( const QString& s );
    void setPixmap( const QPixmap& p );

    void setFilter( const QString& filter );
    void setFilterAvailable( bool b );

    void setUpdaters( const QList<Tomahawk::PlaylistUpdaterInterface*>& updaters );
signals:
    void filterTextChanged( const QString& filter );

protected:
    void changeEvent( QEvent* event );
    void paintEvent( QPaintEvent* event );

private slots:
    void onFilterEdited();
    void artistClicked();

private:
    Ui::InfoBar* ui;

    QPalette m_whitePal;

    QList<Tomahawk::PlaylistUpdaterInterface*> m_updaters;;
    QList<QWidget*> m_updaterConfigurations;

    QSearchField* m_searchWidget;
    QueryLabel* m_queryLabel;
};

#endif // INFOBAR_H
