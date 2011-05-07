/*
    Copyright (C) 2011  Leo Franchi <leo.franchi@kdab.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef SIPCONFIGDELEGATE_H
#define SIPCONFIGDELEGATE_H

#include "configdelegatebase.h"

class SipPlugin;
class SipPluginFactory;
class SipConfigDelegate : public ConfigDelegateBase
{
    Q_OBJECT
public:
    SipConfigDelegate( QObject* parent = 0);

    virtual void paint ( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    virtual bool editorEvent ( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index );
    virtual QSize sizeHint ( const QStyleOptionViewItem& option, const QModelIndex& index ) const;

    virtual QRect checkRectForIndex( const QStyleOptionViewItem &option, const QModelIndex &idx ) const;
    virtual QRect configRectForIndex( const QStyleOptionViewItem& option, const QModelIndex& idx ) const;
private slots:
    void askedForEdit( const QModelIndex& idx );

signals:
    void sipFactoryClicked( SipPluginFactory* );
    void openConfig( SipPlugin* );
};

#endif // SIPCONFIGDELEGATE_H
