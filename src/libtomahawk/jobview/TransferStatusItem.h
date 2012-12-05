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

#ifndef TRANSFERSTATUSITEM_H
#define TRANSFERSTATUSITEM_H

#include "JobStatusItem.h"

#include <QPixmap>

class StreamConnection;

class TransferStatusManager : public QObject
{
    Q_OBJECT
public:
    explicit TransferStatusManager( QObject* parent = 0 );
    virtual ~TransferStatusManager() {}

    QPixmap rxPixmap() const;
    QPixmap txPixmap() const;

private slots:
    void streamRegistered( StreamConnection* sc );
};

class TransferStatusItem : public JobStatusItem
{
    Q_OBJECT
public:
    explicit TransferStatusItem( TransferStatusManager* p, StreamConnection* );
    virtual ~TransferStatusItem();

    virtual QString rightColumnText() const;
    virtual QString mainText() const;
    virtual QPixmap icon() const;
    virtual QString type() const { return m_type; }

private slots:
    void streamFinished( StreamConnection* sc );
    void onTransferUpdate();

private:
    TransferStatusManager* m_parent;
    QString m_type, m_main, m_right;
    QWeakPointer< StreamConnection > m_stream;
};

#endif // TRANSFERSTATUSITEM_H
