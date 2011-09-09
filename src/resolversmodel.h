/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef RESOLVERSMODEL_H
#define RESOLVERSMODEL_H

#include <QModelIndex>
#include <QStringList>


class ResolversModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        ResolverName = Qt::UserRole + 15,
        ResolverPath = Qt::UserRole + 16,
        HasConfig = Qt::UserRole + 17,
        ErrorState = Qt::UserRole + 18
    };

    explicit ResolversModel( QObject* parent = 0 );
    virtual ~ResolversModel();

    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;
    virtual int columnCount( const QModelIndex& parent ) const;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    void addResolver( const QString& resolver, bool enable = false );
    void atticaResolverInstalled ( const QString& resolverId );
    void removeResolver( const QString& resolver );

    void saveScriptResolvers();
private slots:
    void resolverChanged();

private:
    void addInstalledResolvers();
};

#endif // RESOLVERSMODEL_H
