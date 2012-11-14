/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
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

#ifndef DROPJOBNOTIFIER_H
#define DROPJOBNOTIFIER_H

#include "DllMacro.h"
#include "DropJob.h"
#include "Typedefs.h"
#include "Query.h"
#include "jobview/JobStatusItem.h"

#include <QObject>
#include <QSet>
#include <QStringList>
#include <QPixmap>

class NetworkReply;

namespace Tomahawk
{

class DLLEXPORT DropJobNotifier : public JobStatusItem
{
    Q_OBJECT
public:
    DropJobNotifier( QPixmap pixmap, QString service, DropJob::DropType type, NetworkReply* job );

    // No QNetworkReply, needs manual finished call
    DropJobNotifier( QPixmap pixmap, DropJob::DropType type );
    virtual ~DropJobNotifier();

    virtual QString rightColumnText() const;
    virtual QString mainText() const;
    virtual QPixmap icon() const;
    virtual QString type() const { return m_type; }
    virtual bool collapseItem() const { return true; }

public slots:
    void setFinished();

private:
    void init( DropJob::DropType type );

    QString m_type;
    NetworkReply* m_job;
    QPixmap m_pixmap;
    QString m_service;
};

}

#endif // DROPJOBNOTIFIER_H
