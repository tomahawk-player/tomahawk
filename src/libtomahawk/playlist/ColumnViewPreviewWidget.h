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

#ifndef COLUMNVIEWPREVIEWWIDGET_H
#define COLUMNVIEWPREVIEWWIDGET_H

#include <QWidget>

#include "Query.h"
#include "DllMacro.h"

class ColumnView;

namespace Ui
{
    class ColumnViewPreviewWidget;
}

class DLLEXPORT ColumnViewPreviewWidget : public QWidget
{
Q_OBJECT

public:
    explicit ColumnViewPreviewWidget( ColumnView* parent );
    ~ColumnViewPreviewWidget();

    QSize minimumSize() const;

public slots:
    void setQuery( const Tomahawk::query_ptr& query );

protected:
    void changeEvent( QEvent* e );

private slots:
    void onCoverUpdated();

private:
    Ui::ColumnViewPreviewWidget* ui;
    Tomahawk::query_ptr m_query;
};

#endif // COLUMNVIEWPREVIEWWIDGET_H
