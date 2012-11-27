/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright      2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef CHECKDIRTREE_H
#define CHECKDIRTREE_H

#include "DllMacro.h"

#include <QFileSystemModel>
#include <QTreeView>

class DLLEXPORT CheckDirModel : public QFileSystemModel
{
    Q_OBJECT

public:
    CheckDirModel( QWidget* parent = 0 );
    virtual ~CheckDirModel();

    virtual Qt::ItemFlags flags( const QModelIndex& index ) const;
    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );

    void setCheck( const QModelIndex& index, const QVariant& value );
    Qt::CheckState getCheck( const QModelIndex& index );

    void cleanup();
signals:
    void dataChangedByUser( const QModelIndex & index );

private slots:
    void getFileInfoResult();
    void volumeShowFinished();
    void processErrorOutput();
private:
    QHash<QPersistentModelIndex, Qt::CheckState> m_checkTable;

    bool m_shownVolumes;
    QString m_setFilePath;
    QString m_getFileInfoPath;
};


class DLLEXPORT CheckDirTree : public QTreeView
{
    Q_OBJECT

public:
    CheckDirTree( QWidget* parent );

    void checkPath( const QString& path, Qt::CheckState state );

    void setExclusions( const QStringList& list );
    QStringList getExclusions();
    QStringList getCheckedPaths();

    void cleanup();
signals:
    void changed();

private slots:
    void onCollapse( const QModelIndex& idx );
    void onExpand( const QModelIndex& idx );
    void updateNode( const QModelIndex& idx );

    void modelReset();
private:
    CheckDirModel m_dirModel;
    QSet<qint64> m_expandedSet;

    void fillDown( const QModelIndex& index );
    void updateParent( const QModelIndex& index );
    void getExclusionsForNode( const QModelIndex& index, QStringList& exclusions );
    void getChecksForNode( const QModelIndex& index, QStringList& checks );
};

#endif // CHECKDIRTREE_H
