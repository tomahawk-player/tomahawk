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

#ifndef JOBSTATUSMODEL_H
#define JOBSTATUSMODEL_H

#include "DllMacro.h"

#include <QModelIndex>
#include <QSortFilterProxyModel>
#include <QQueue>

class QStyledItemDelegate;
class JobStatusItem;

class DLLEXPORT JobStatusModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum JobRoles {
        // DecorationRole is icon
        // DisplayRole is main col
        RightColumnRole = Qt::UserRole + 1,
        AllowMultiLineRole = Qt::UserRole + 2,
        JobDataRole = Qt::UserRole + 3,
        SortRole = Qt::UserRole + 4,
        AgeRole = Qt::UserRole + 5
    };

    explicit JobStatusModel( QObject* parent = 0 );
    virtual ~JobStatusModel();

    virtual Qt::ItemFlags flags( const QModelIndex& index ) const;
    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;

signals:
    void customDelegateJobInserted( int row, JobStatusItem* item );
    void customDelegateJobRemoved( int row );
    void refreshDelegates();

public slots:
    /// Takes ownership of job
    void addJob( JobStatusItem* item );

private slots:
    void itemUpdated();
    void itemFinished();

private:
    QList< JobStatusItem* > m_items;
    QHash< QString, QList< JobStatusItem* > > m_collapseCount;
    QHash< QString, QQueue< JobStatusItem* > > m_jobQueue;
    QHash< QString, int > m_jobTypeCount;
};

class DLLEXPORT JobStatusSortModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    JobStatusSortModel( QObject* parent = 0 );
    virtual ~JobStatusSortModel();

    void setJobModel( JobStatusModel* model );

signals:
    void checkCount();
    void customDelegateJobInserted( int row, JobStatusItem* item );
    void customDelegateJobRemoved( int row );
    void refreshDelegates();

public slots:
    void addJob( JobStatusItem* item );
    void customDelegateJobInsertedSlot( int row, JobStatusItem* item);
    void customDelegateJobRemovedSlot( int row );
    void refreshDelegatesSlot();

protected:
    virtual bool lessThan( const QModelIndex & left, const QModelIndex & right ) const;

private:
    JobStatusModel* m_sourceModel;
};

#endif // JOBSTATUSMODEL_H
