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
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GETNEWSTUFFMODEL_H
#define GETNEWSTUFFMODEL_H

#include <QModelIndex>

#include <attica/content.h>

class GetNewStuffModel: public QAbstractListModel
{
    Q_OBJECT
public:
    enum NewStuffRoles {
        // DisplayRole is title
        // DecorationRole is qicon for item
        DownloadUrlRole = Qt::UserRole + 1,
        RatingRole = Qt::UserRole + 2,
        DownloadCounterRole = Qt::UserRole + 3,
        VersionRole = Qt::UserRole + 4,
        DescriptionRole = Qt::UserRole + 5,
        TypeRole = Qt::UserRole + 6, // Category in attica-speak. What sort of item this is (resolver, etc).
        AuthorRole = Qt::UserRole + 7,
        StateRole = Qt::UserRole + 8
    };

    enum Types {
        ResolverType = 0,
    };

    enum States {
        Uninstalled = 0,
        Installing = 1,
        Failed = 2,
        Installed = 3
    };

    explicit GetNewStuffModel( QObject* parent = 0 );
    virtual ~GetNewStuffModel();

    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;
    virtual bool setData( const QModelIndex &index, const QVariant &value, int role );

private slots:
    void resolversReloaded( const Attica::Content::List& );

private:
    bool m_clicked;

    Attica::Content::List m_contentList;
};

#endif // GETNEWSTUFFMODEL_H
