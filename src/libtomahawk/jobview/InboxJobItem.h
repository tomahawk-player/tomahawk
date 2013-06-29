/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#ifndef INBOXJOBITEM_H
#define INBOXJOBITEM_H

#include "DllMacro.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusItem.h"


class DLLEXPORT InboxJobItem : public JobStatusItem
{
    Q_OBJECT
public:
    enum Side
    {
        Sending = 0,
        Receiving = 1
    };

    explicit InboxJobItem( Side side,
                           const QString& prettyName,
                           const Tomahawk::trackdata_ptr& track,
                           QObject* parent = 0 );
    virtual ~InboxJobItem();

    virtual QString rightColumnText() const { return QString(); }
    virtual QString mainText() const;
    virtual QPixmap icon() const;
    virtual QString type() const { return "inboxjob"; }

    bool allowMultiLine() const { return true; }

private:
    Tomahawk::trackdata_ptr m_track;
    QString m_prettyName;

    QTimer* m_timer;
    static QPixmap* s_pixmap;
    Side m_side;
};

#endif // INBOXJOBITEM_H
