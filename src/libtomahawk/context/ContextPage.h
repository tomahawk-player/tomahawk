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

#ifndef CONTEXTPAGE_H
#define CONTEXTPAGE_H

#include <QGraphicsProxyWidget>
#include <QGraphicsWebView>
#include <QStyleOptionGraphicsItem>

#include "typedefs.h"
#include "playlistinterface.h"
#include "utils/stylehelper.h"
#include "utils/tomahawkutils.h"

#include "dllmacro.h"
#include <signal.h>

namespace Tomahawk
{

class DLLEXPORT ContextPage : public QObject
{
    Q_OBJECT

public:
    ContextPage() {}
    virtual ~ContextPage() {}

    virtual QGraphicsWidget* widget() = 0;
    virtual Tomahawk::PlaylistInterface* playlistInterface() const = 0;

    virtual QString title() const = 0;
    virtual QString description() const = 0;
    virtual QPixmap pixmap() const { return QPixmap( RESPATH "icons/tomahawk-icon-128x128.png" ); }

    virtual bool jumpToCurrentTrack() = 0;

public slots:
    virtual void setQuery( const Tomahawk::query_ptr& query ) = 0;

signals:
    void nameChanged( const QString& );
    void descriptionChanged( const QString& );
    void pixmapChanged( const QPixmap& );
    void destroyed( QWidget* widget );
};


class DLLEXPORT ContextProxyPage : public QGraphicsWidget
{
    Q_OBJECT

public:
    ContextProxyPage() : QGraphicsWidget()
    {}

    Tomahawk::ContextPage* page() const { return m_page; }
    void setPage( Tomahawk::ContextPage* page );

    virtual bool eventFilter( QObject* watched, QEvent* event );

signals:
    void focused();

protected:
    virtual void paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget );
    virtual bool sceneEvent( QEvent* event );

private:
    Tomahawk::ContextPage* m_page;
};

}; // ns

#endif //CONTEXTPAGE_H
