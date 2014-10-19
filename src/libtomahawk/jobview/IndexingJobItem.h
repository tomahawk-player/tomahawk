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

#ifndef INDEXINGJOBITEM_H
#define INDEXINGJOBITEM_H

#include "JobStatusItem.h"

#include <QPointer>

class IndexingJobItem : public JobStatusItem
{
    Q_OBJECT
public:
    explicit IndexingJobItem() {}

    void done();

    virtual int weight() const { return 50; }
    virtual QString rightColumnText() const { return QString(); }
    virtual QString mainText() const;
    virtual QPixmap icon() const;
    virtual QString type() const { return "indexerjob"; }
};

class IndexStatusManager : public QObject
{
    Q_OBJECT
public:
    explicit IndexStatusManager( QObject* parent = 0 );
    virtual ~IndexStatusManager() {}

private slots:
    void started();
    void finished();

private:
    QPointer<IndexingJobItem> m_curItem;
};

#endif // INDEXINGJOBITEM_H
