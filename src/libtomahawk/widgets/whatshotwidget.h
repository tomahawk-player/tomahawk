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

#ifndef WHATSHOTWIDGET_H
#define WHATSHOTWIDGET_H

#include <QWidget>
#include <QListWidgetItem>
#include <QStyledItemDelegate>

#include "playlistinterface.h"
#include "infosystem/infosystem.h"
#include "playlist.h"
#include "result.h"
#include "viewpage.h"

#include "utils/tomahawkutils.h"

#include "dllmacro.h"

class QStandardItemModel;
class QStandardItem;
class TreeModel;
class PlaylistModel;
class OverlayWidget;
class TreeProxyModel;

namespace Ui
{
    class WhatsHotWidget;
}

/**
 * \class
 * \brief The tomahawk page that shows music charts.
 */
class DLLEXPORT WhatsHotWidget : public QWidget, public Tomahawk::ViewPage
{
Q_OBJECT

public:
    WhatsHotWidget( QWidget* parent = 0 );
    ~WhatsHotWidget();

    virtual QWidget* widget() { return this; }
    virtual Tomahawk::PlaylistInterface* playlistInterface() const { return 0; }

    virtual QString title() const { return tr( "Charts" ); }
    virtual QString description() const { return QString(); }

    virtual bool showStatsBar() const { return false; }
    virtual bool showInfoBar() const { return false; }

    virtual bool jumpToCurrentTrack() { return false; }

protected:
    void changeEvent( QEvent* e );

signals:
    void destroyed( QWidget* widget );

public slots:

private slots:
    void fetchData();
    void checkQueries();
    void infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );
    void infoSystemFinished( QString target );

private:
    QStandardItem* parseNode(QStandardItem* parentItem, const QString &label, const QVariant &data);
    Ui::WhatsHotWidget *ui;

    PlaylistModel* m_tracksModel;
    TreeModel* m_artistsModel;
    TreeProxyModel* m_artistsProxy;
    QStandardItemModel* m_crumbModelLeft;

    QTimer* m_timer;
};

#endif // WHATSHOTWIDGET_H
