/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#ifndef TREEWIDGET_H
#define TREEWIDGET_H

#include "TreeView.h"
#include "ViewPage.h"
#include "widgets/ScriptCollectionHeader.h"

#include <QWidget>

class DLLEXPORT TreeWidget : public QWidget, public Tomahawk::ViewPage
{
    Q_OBJECT
public:
    explicit TreeWidget( QWidget* parent = 0 );
    virtual ~TreeWidget();

    TreeView* view() const;

    virtual QWidget* widget();
    virtual Tomahawk::playlistinterface_ptr playlistInterface() const;

    virtual QString title() const;
    virtual QString description() const;
    virtual QPixmap pixmap() const;

    virtual bool showFilter() const;
    virtual bool jumpToCurrentTrack();

    virtual bool showInfoBar() const;

public slots:
    virtual bool setFilter( const QString& filter );

private slots:
    void onModelChanged();
    void onRefreshClicked();

private:
    TreeView* m_view;
    ScriptCollectionHeader* m_header;
};

#endif // TREEWIDGET_H
