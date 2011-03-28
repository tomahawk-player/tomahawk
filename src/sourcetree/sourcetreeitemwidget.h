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

#ifndef SOURCETREEITEMWIDGET_H
#define SOURCETREEITEMWIDGET_H

#include <QWidget>

#include "source.h"

namespace Ui
{
    class SourceTreeItemWidget;
}

class SourceTreeItemWidget : public QWidget
{
Q_OBJECT

public:
    SourceTreeItemWidget( const Tomahawk::source_ptr& source, QWidget* parent = 0 );
    ~SourceTreeItemWidget();

signals:
    void clicked();

public slots:
    void onOnline();
    void onOffline();

protected:
    void changeEvent( QEvent* e );

private slots:
    void gotStats( const QVariantMap& stats );
    void onLoadingStateChanged( DBSyncConnection::State newstate, DBSyncConnection::State oldstate, const QString& info );

    void onPlaybackStarted( const Tomahawk::query_ptr& query );

    void onInfoButtonClicked();

private:
    Tomahawk::source_ptr m_source;

    Ui::SourceTreeItemWidget* ui;
};

#endif // SOURCETREEITEMWIDGET_H
