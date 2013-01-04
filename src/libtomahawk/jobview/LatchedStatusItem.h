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

#ifndef LATCHEDSTATUSITEM_H
#define LATCHEDSTATUSITEM_H

#include "JobStatusItem.h"
#include "Typedefs.h"

#include <QHash>
#include <QPixmap>

class LatchedStatusManager;

class LatchedStatusItem : public JobStatusItem
{
    Q_OBJECT
public:
    explicit LatchedStatusItem( const Tomahawk::source_ptr& from, const Tomahawk::source_ptr& to, LatchedStatusManager* );
    virtual ~LatchedStatusItem();

    virtual QString rightColumnText() const { return QString(); }
    virtual QString mainText() const;
    virtual QPixmap icon() const;
    virtual QString type() const;

    void stop();
private:
    Tomahawk::source_ptr m_from, m_to;
    QString m_text;
    LatchedStatusManager* m_parent;
};

class LatchedStatusManager : public QObject
{
    Q_OBJECT
public:
    explicit LatchedStatusManager( QObject* parent = 0 );
    virtual ~LatchedStatusManager() {}

    QPixmap pixmap() const;

private slots:
    void latchedOn( const Tomahawk::source_ptr&, const Tomahawk::source_ptr& );
    void latchedOff( const Tomahawk::source_ptr&, const Tomahawk::source_ptr& );
    void sourceOffline();

private:
    QHash< QString, QPointer< LatchedStatusItem > > m_jobs;
};

#endif // LATCHEDSTATUSITEM_H
