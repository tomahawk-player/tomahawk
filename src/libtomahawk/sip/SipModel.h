/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>

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


#ifndef SIPMODEL_H
#define SIPMODEL_H

#include "dllmacro.h"

#include <QModelIndex>
#include <QStringList>

class SipPlugin;

class DLLEXPORT SipModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum Roles {
        PluginName = Qt::UserRole + 15,
        ConnectionStateRole = Qt::UserRole + 17,
        HasConfig = Qt::UserRole + 18,
        FactoryRole = Qt::UserRole + 19,
        ErrorString = Qt::UserRole + 20,
        FactoryItemRole = Qt::UserRole + 21,
        FactoryItemIcon = Qt::UserRole + 22,
        SipPluginData = Qt::UserRole + 23,
        SipPluginFactoryData = Qt::UserRole + 24
    };

    explicit SipModel( QObject* parent = 0 );
    virtual ~SipModel();

    virtual QModelIndex index ( int row, int column, const QModelIndex& parent = QModelIndex() ) const;
    virtual QModelIndex parent ( const QModelIndex& child ) const;
    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;
    virtual int columnCount( const QModelIndex& parent ) const;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

private slots:
    void pluginAdded( SipPlugin* p );
    void pluginRemoved( SipPlugin* p );
    void pluginStateChanged( SipPlugin* p );
};

#endif // SIPMODEL_H
